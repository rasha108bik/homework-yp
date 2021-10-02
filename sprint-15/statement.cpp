#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    return closure[var_] = std::move(rv_.get()->Execute(closure, context));
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    :var_(std::move(var)), rv_(std::move(rv)) {
}

VariableValue::VariableValue(const std::string& var_name) {
    dotted_ids_.push_back(var_name);
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids)
    :dotted_ids_(std::move(dotted_ids)) {
}

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    Closure* clo = &closure;
    auto it = closure.begin();
    for (const std::string& str : dotted_ids_) {
        it = clo->find(str);
        if (it == clo->end()) {
            throw std::runtime_error("ERROR VariableValue::Execute()");
        }
        auto class_ = it->second.TryAs<runtime::ClassInstance>();
        if (class_ != nullptr) {
            clo = &class_->Fields();
        }
    }

    return it->second;
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return std::make_unique<Print>(std::make_unique<VariableValue>(name));
}

Print::Print(unique_ptr<Statement> argument) {
    args_.push_back(std::move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    :args_(std::move(args)) {
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    bool start = true;
    auto& out = context.GetOutputStream();
    ObjectHolder obj_hol;
    for (const auto& arg : args_) {
        if (!start) {
            out << " "s;
        }
        obj_hol = arg->Execute(closure, context);
        if (obj_hol.operator bool()) {
            obj_hol->Print(out, context);
        }
        else {
            out << "None"s;
        }
        start = false;
    }
    out << "\n"s;
    return obj_hol;
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method, std::vector<std::unique_ptr<Statement>> args)
    :object_(std::move(object)), method_(std::move(method)), args_(std::move(args)) {
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    std::vector<runtime::ObjectHolder> vec;
    for (const auto& obj : args_) {
        vec.push_back(obj->Execute(closure, context));
    }
    return object_->Execute(closure, context).TryAs< runtime::ClassInstance>()->Call(method_, vec, context);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    auto arg = argument_->Execute(closure, context);
    if (!arg.operator bool()) {
        return ObjectHolder::Own(runtime::String{ "None"s });
    }

    std::ostringstream for_print;
    arg->Print(for_print, context);
    return ObjectHolder::Own(runtime::String{ for_print.str() });
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    auto obj1 = lhs_->Execute(closure, context);
    auto obj2 = rhs_->Execute(closure, context);

    auto obj1_num = obj1.TryAs<runtime::Number>();
    auto obj2_num = obj2.TryAs<runtime::Number>();
    if (obj1_num != nullptr && obj2_num != nullptr) {
        return ObjectHolder::Own(runtime::Number(obj1_num->GetValue() + obj2_num->GetValue()));
    }
    auto obj1_str = obj1.TryAs<runtime::String>();
    auto obj2_str = obj2.TryAs<runtime::String>();
    if (obj1_str != nullptr && obj2_str != nullptr) {
        return ObjectHolder::Own(runtime::String(obj1_str->GetValue() + obj2_str->GetValue()));
    }

    auto obj1_class = obj1.TryAs<runtime::ClassInstance>();
    if (obj1_class != nullptr) {
        if (obj1_class->HasMethod(ADD_METHOD, 1)) {
            return obj1_class->Call(ADD_METHOD, { obj2 }, context);
        }
    }
    throw std::runtime_error("ERROR Add::Execute()"s);
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    auto obj1 = lhs_->Execute(closure, context);
    auto obj2 = rhs_->Execute(closure, context);

    auto obj1_num = obj1.TryAs<runtime::Number>();
    auto obj2_num = obj2.TryAs<runtime::Number>();
    if (obj1_num != nullptr && obj2_num != nullptr) {
        return ObjectHolder::Own(runtime::Number(obj1_num->GetValue() - obj2_num->GetValue()));
    }
    throw std::runtime_error("ERROR Sub::Execute()"s);
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    auto obj1 = lhs_->Execute(closure, context);
    auto obj2 = rhs_->Execute(closure, context);

    auto obj1_num = obj1.TryAs<runtime::Number>();
    auto obj2_num = obj2.TryAs<runtime::Number>();
    if (obj1_num != nullptr && obj2_num != nullptr) {
        return ObjectHolder::Own(runtime::Number(obj1_num->GetValue() * obj2_num->GetValue()));
    }
    throw std::runtime_error("ERROR Sub::Execute()"s);
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    auto obj1 = lhs_->Execute(closure, context);
    auto obj2 = rhs_->Execute(closure, context);

    auto obj1_num = obj1.TryAs<runtime::Number>();
    auto obj2_num = obj2.TryAs<runtime::Number>();
    if (obj1_num != nullptr && obj2_num != nullptr && obj2_num->GetValue() != 0) {
        return ObjectHolder::Own(runtime::Number(obj1_num->GetValue() / obj2_num->GetValue()));
    }
    throw std::runtime_error("ERROR Sub::Execute()"s);
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (const auto& arg : args_) {
        arg->Execute(closure, context);
    }
    return {};
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_->Execute(closure, context);
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
    :cls_(std::move(cls)) {
}

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    return closure[cls_.TryAs<runtime::Class>()->GetName()] = std::move(cls_);
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv)
    :object_(std::move(object)), field_name_(std::move(field_name)), rv_(std::move(rv)) {
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    return object_.Execute(closure, context).TryAs<runtime::ClassInstance>()->Fields()[field_name_] = std::move(rv_->Execute(closure, context));
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body, std::unique_ptr<Statement> else_body)
    :condition_(std::move(condition)), if_body_(std::move(if_body)), else_body_(std::move(else_body)) {
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(condition_->Execute(closure, context))) {
        return if_body_->Execute(closure, context);
    }
    if (else_body_) {
        return else_body_->Execute(closure, context);
    }
    else {
        return {};
    }
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    if (IsTrue(lhs_->Execute(closure, context)) ||
    IsTrue(rhs_->Execute(closure, context))) {
        return ObjectHolder::Own(runtime::Bool(true));
    }
    return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    if (IsTrue(lhs_->Execute(closure, context)) &&
    IsTrue(rhs_->Execute(closure, context))) {
        return ObjectHolder::Own(runtime::Bool(true));
    }

    return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    if (!IsTrue(argument_->Execute(closure, context))) {
        return ObjectHolder::Own(runtime::Bool(true));
    }
    return ObjectHolder::Own(runtime::Bool(false));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(std::move(cmp)) {
}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    bool result = cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context);
    return ObjectHolder::Own(runtime::Bool(result));
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
    : class_ins_(class_), args_(std::move(args)) {
}

NewInstance::NewInstance(const runtime::Class& class_)
    : class_ins_(class_) {
}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    if (class_ins_.HasMethod(INIT_METHOD, args_.size())) {
        std::vector < runtime::ObjectHolder> vec;
        vec.reserve(args_.size());
        for (const auto& arg : args_) {
            vec.push_back(arg->Execute(closure, context));
        }
        class_ins_.Call(INIT_METHOD, std::move(vec), context);
    }
    return runtime::ObjectHolder::Share(class_ins_);
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body)
    : body_(std::move(body)) {
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        body_->Execute(closure, context);
    }
    catch (runtime::ObjectHolder obj) {
        return obj;
    }

    return {};
}

}  // namespace ast