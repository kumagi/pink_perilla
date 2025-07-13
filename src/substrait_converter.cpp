#include "substrait_converter.hpp"

#include "substrait/algebra.pb.h"
#include "substrait/type.pb.h"


#include "utils.hpp"

namespace pink_perilla::converter {


substrait::Plan ToSubstrait(const SelectInfo& info) {
    substrait::Plan plan;
    substrait::Rel* current_rel;

    if (info.from_subquery) {
        substrait::Plan sub_plan = ToSubstrait(**info.from_subquery);
        if (sub_plan.relations_size() > 0 && sub_plan.relations(0).has_root()) {
            current_rel = new substrait::Rel(sub_plan.relations(0).root().input());
        } else {
            return substrait::Plan();
        }
    } else if (info.from_table) {
        substrait::ReadRel* read_rel = new substrait::ReadRel();
        read_rel->mutable_named_table()->add_names(*info.from_table);
        current_rel = new substrait::Rel();
        current_rel->set_allocated_read(read_rel);
    } else {
        return substrait::Plan();
    }

    for (const auto& cross_join_table : info.cross_join_tables) {
        substrait::CrossRel* cross_rel = new substrait::CrossRel();
        cross_rel->set_allocated_left(current_rel);
        substrait::ReadRel* right_read = new substrait::ReadRel();
        right_read->mutable_named_table()->add_names(cross_join_table);
        substrait::Rel* right_rel = new substrait::Rel();
        right_rel->set_allocated_read(right_read);
        cross_rel->set_allocated_right(right_rel);
        current_rel = new substrait::Rel();
        current_rel->set_allocated_cross(cross_rel);
    }

    for (const auto& join_info : info.joins) {
        substrait::JoinRel* join_rel = new substrait::JoinRel();
        if (join_info.type == JoinType::INNER)
            join_rel->set_type(substrait::JoinRel::JOIN_TYPE_INNER);
        else
            join_rel->set_type(substrait::JoinRel::JOIN_TYPE_LEFT);
        join_rel->set_allocated_left(current_rel);
        join_rel->set_allocated_right(new substrait::Rel());
        join_rel->mutable_right()->set_allocated_read(new substrait::ReadRel());
        join_rel->mutable_right()->mutable_read()->mutable_named_table()->add_names(join_info.table);
        substrait::Expression* condition = new substrait::Expression();
        substrait::Expression::ScalarFunction* scalar_fn = condition->mutable_scalar_function();
        scalar_fn->set_function_reference(0);
        substrait::FunctionArgument* arg = scalar_fn->add_arguments();
        arg->mutable_value()->mutable_literal()->set_string(join_info.on_condition);
        join_rel->set_allocated_expression(condition);
        current_rel = new substrait::Rel();
        current_rel->set_allocated_join(join_rel);
    }

    bool has_aggregates = false;
    for (const auto& item : info.select_items) {
        if (item.type == SelectItemType::AGGREGATE_FUNCTION) {
            has_aggregates = true;
            break;
        }
    }

    if (has_aggregates) {
        substrait::AggregateRel* agg_rel = new substrait::AggregateRel();
        agg_rel->set_allocated_input(current_rel);
        if (!info.group_by_columns.empty()) {
            substrait::AggregateRel::Grouping* grouping = agg_rel->add_groupings();
            for (uint32_t i = 0; i < info.group_by_columns.size(); ++i) {
                substrait::Expression* expr = agg_rel->add_grouping_expressions();
                expr->mutable_literal()->set_string(info.group_by_columns[i]);
                grouping->add_expression_references(i);
            }
        }
        for (const auto& item : info.select_items) {
            if (item.type == SelectItemType::AGGREGATE_FUNCTION) {
                substrait::AggregateRel::Measure* measure = agg_rel->add_measures();
                measure->mutable_measure()->set_function_reference(0);
            }
        }
        current_rel = new substrait::Rel();
        current_rel->set_allocated_aggregate(agg_rel);
    }

    std::vector<WindowFunctionInfo> window_functions;
    for (const auto& item : info.select_items) {
        if (item.type == SelectItemType::WINDOW_FUNCTION && item.win_info) {
            window_functions.push_back(*item.win_info);
        }
    }

    if (!window_functions.empty()) {
        substrait::ConsistentPartitionWindowRel* window_rel = new substrait::ConsistentPartitionWindowRel();
        window_rel->set_allocated_input(current_rel);
        if (!window_functions.front().partition_by.empty()) {
            for (const auto& col : window_functions.front().partition_by) {
                substrait::Expression* expr = window_rel->add_partition_expressions();
                expr->mutable_literal()->set_string(col);
            }
        }
        for (const auto& win_func_info : window_functions) {
            substrait::ConsistentPartitionWindowRel::WindowRelFunction* win_func = window_rel->add_window_functions();
            win_func->set_function_reference(0);
        }
        current_rel = new substrait::Rel();
        current_rel->set_allocated_window(window_rel);
    }

    bool is_select_star = info.select_items.size() == 1 && info.select_items[0].expression == "*";

    if (!is_select_star) {
        substrait::ProjectRel* project_rel = new substrait::ProjectRel();
        project_rel->set_allocated_input(current_rel);
        for (const auto& item : info.select_items) {
            if (item.type != SelectItemType::WINDOW_FUNCTION) {
                substrait::Expression* expr = project_rel->add_expressions();
                expr->mutable_literal()->set_string(item.expression);
            }
        }
        if (project_rel->expressions_size() > 0) {
            current_rel = new substrait::Rel();
            current_rel->set_allocated_project(project_rel);
        } else {
            delete project_rel;
        }
    }

    if (!info.order_by_columns.empty()) {
        substrait::SortRel* sort_rel = new substrait::SortRel();
        sort_rel->set_allocated_input(current_rel);
        for (const auto& sort_info : info.order_by_columns) {
            substrait::SortField* sort_field = sort_rel->add_sorts();
            sort_field->set_direction(sort_info.direction);
            sort_field->mutable_expr()->mutable_literal()->set_string(sort_info.column);
        }
        current_rel = new substrait::Rel();
        current_rel->set_allocated_sort(sort_rel);
    }

    if (info.limit != -1) {
        substrait::FetchRel* fetch_rel = new substrait::FetchRel();
        fetch_rel->set_allocated_input(current_rel);
        fetch_rel->mutable_count_expr()->mutable_literal()->set_i64(info.limit);
        current_rel = new substrait::Rel();
        current_rel->set_allocated_fetch(fetch_rel);
    }

    substrait::RelRoot* root = new substrait::RelRoot();
    root->set_allocated_input(current_rel);
    plan.add_relations()->set_allocated_root(root);
    return plan;
}

substrait::Plan ToSubstrait(const CreateTableInfo& info) {
    substrait::Plan plan;
    substrait::DdlRel* ddl_rel = new substrait::DdlRel();
    ddl_rel->set_op(substrait::DdlRel::DDL_OP_CREATE);
    ddl_rel->set_object(substrait::DdlRel::DDL_OBJECT_TABLE);
    ddl_rel->mutable_named_object()->add_names(info.table_name);
    substrait::NamedStruct* schema = ddl_rel->mutable_table_schema();
    for (const auto& col : info.columns) {
        schema->add_names(col.name);
        substrait::Type* type =
            schema->mutable_struct_()->mutable_types()->Add();
        if (col.type == "integer") {
            type->mutable_i32()->set_nullability(
                substrait::Type::NULLABILITY_NULLABLE);
        } else {
            type->mutable_string()->set_nullability(
                substrait::Type::NULLABILITY_NULLABLE);
        }
    }
    substrait::Rel* rel = new substrait::Rel();
    rel->set_allocated_ddl(ddl_rel);
    substrait::RelRoot* root = new substrait::RelRoot();
    root->set_allocated_input(rel);
    plan.add_relations()->set_allocated_root(root);
    return plan;
}

substrait::Plan ToSubstrait(const DropTableInfo& info) {
    substrait::Plan plan;
    substrait::DdlRel* ddl_rel = new substrait::DdlRel();
    ddl_rel->set_op(substrait::DdlRel::DDL_OP_DROP);
    ddl_rel->set_object(substrait::DdlRel::DDL_OBJECT_TABLE);
    ddl_rel->mutable_named_object()->add_names(info.table_name);
    substrait::Rel* rel = new substrait::Rel();
    rel->set_allocated_ddl(ddl_rel);
    substrait::RelRoot* root = new substrait::RelRoot();
    root->set_allocated_input(rel);
    plan.add_relations()->set_allocated_root(root);
    return plan;
}

substrait::Plan ToSubstrait(const DeleteInfo& info) {
    substrait::Plan plan;
    substrait::WriteRel* write_rel = new substrait::WriteRel();
    write_rel->set_op(substrait::WriteRel::WRITE_OP_DELETE);
    write_rel->mutable_named_table()->add_names(info.table_name);

    substrait::ReadRel* read_rel = new substrait::ReadRel();
    read_rel->mutable_named_table()->add_names(info.table_name);

    substrait::Rel* input_rel = new substrait::Rel();
    input_rel->set_allocated_read(read_rel);

    if (info.where_clause) {
        substrait::FilterRel* filter_rel = new substrait::FilterRel();
        filter_rel->set_allocated_input(input_rel);

        substrait::Expression* condition = filter_rel->mutable_condition();
        substrait::Expression::ScalarFunction* scalar_fn =
            condition->mutable_scalar_function();
        scalar_fn->set_function_reference(0);  // Placeholder
        substrait::FunctionArgument* arg = scalar_fn->add_arguments();
        arg->mutable_value()->mutable_literal()->set_string(
            *info.where_clause);

        substrait::Rel* filter_rel_container = new substrait::Rel();
        filter_rel_container->set_allocated_filter(filter_rel);
        write_rel->set_allocated_input(filter_rel_container);
    } else {
        write_rel->set_allocated_input(input_rel);
    }

    substrait::Rel* rel = new substrait::Rel();
    rel->set_allocated_write(write_rel);
    substrait::RelRoot* root = new substrait::RelRoot();
    root->set_allocated_input(rel);
    plan.add_relations()->set_allocated_root(root);
    return plan;
}

substrait::Plan ToSubstrait(const UpdateInfo& info) {
    substrait::Plan plan;
    substrait::UpdateRel* update_rel = new substrait::UpdateRel();
    update_rel->mutable_named_table()->add_names(info.table_name);

    if (info.where_clause) {
        substrait::Expression* condition = update_rel->mutable_condition();
        substrait::Expression::ScalarFunction* scalar_fn =
            condition->mutable_scalar_function();
        scalar_fn->set_function_reference(0);  // Placeholder
        substrait::FunctionArgument* arg = scalar_fn->add_arguments();
        arg->mutable_value()->mutable_literal()->set_string(
            *info.where_clause);
    }

    int col_idx = 0;
    for (const auto& set_clause : info.set_clauses) {
        substrait::UpdateRel::TransformExpression* transform =
            update_rel->add_transformations();
        transform->set_column_target(
            col_idx++);  // FIXME: This is a placeholder
        substrait::Expression* literal_expr =
            transform->mutable_transformation();
        substrait::Expression::Literal* literal =
            literal_expr->mutable_literal();
        try {
            literal->set_i32(std::stoi(set_clause.value));
        } catch (...) {
            std::string s = set_clause.value;
            if (s.front() == '\'' && s.back() == '\'') {
                s = s.substr(1, s.length() - 2);
            }
            literal->set_string(s);
        }
    }

    substrait::Rel* rel = new substrait::Rel();
    rel->set_allocated_update(update_rel);
    substrait::RelRoot* root = new substrait::RelRoot();
    root->set_allocated_input(rel);
    plan.add_relations()->set_allocated_root(root);
    return plan;
}

substrait::Plan ToSubstrait(const InsertInfo& info) {
    substrait::Plan plan;
    substrait::WriteRel* write_rel = new substrait::WriteRel();
    write_rel->mutable_named_table()->add_names(info.table_name);

    substrait::ReadRel* read_rel = new substrait::ReadRel();
    substrait::ReadRel::VirtualTable* virtual_table =
        read_rel->mutable_virtual_table();
    substrait::Expression::Nested::Struct* row =
        virtual_table->add_expressions();

    for (const auto& val : info.values) {
        substrait::Expression* field = row->add_fields();
        if (auto int_val = utils::StringToIntOptional(val)) {
            field->mutable_literal()->set_i32(*int_val);
        } else {
            std::string s = val;
            if (s.front() == '\'' && s.back() == '\'') {
                s = s.substr(1, s.length() - 2);
            }
            field->mutable_literal()->set_string(s);
        }
    }

    substrait::NamedStruct* schema = read_rel->mutable_base_schema();
    for (const auto& col : info.columns) {
        schema->add_names(col);
    }

    substrait::Rel* input_rel = new substrait::Rel();
    input_rel->set_allocated_read(read_rel);
    write_rel->set_allocated_input(input_rel);

    substrait::Rel* rel = new substrait::Rel();
    rel->set_allocated_write(write_rel);

    substrait::RelRoot* root = new substrait::RelRoot();
    root->set_allocated_input(rel);
    plan.add_relations()->set_allocated_root(root);

    return plan;
}


}  // namespace pink_perilla::converter
