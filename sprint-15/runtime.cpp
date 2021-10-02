#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <algorithm>

using namespace std;

namespace runtime {

    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
    }

    void ObjectHolder::AssertIsValid() const {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object& object) {
        // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
    }

    ObjectHolder ObjectHolder::None() {
        return ObjectHolder();
    }

    Object& ObjectHolder::operator*() const {
        AssertIsValid();
        return *Get();
    }

    Object* ObjectHolder::operator->() const {
        AssertIsValid();
        return Get();
    }

    Object* ObjectHolder::Get() const {
        return data_.get();
    }

    ObjectHolder::operator bool() const {
        return Get() != nullptr;
    }

    bool IsTrue(const ObjectHolder& object) {
        if (object.operator bool()) {
            if (
                    (object.TryAs<Bool>() != nullptr && object.TryAs<Bool>()->GetValue()) ||
                    (object.TryAs<String>() != nullptr && !object.TryAs<String>()->GetValue().empty()) ||
                    (object.TryAs<Number>() != nullptr && object.TryAs<Number>()->GetValue())
                    ) {
                return true;
            }
        }

        auto ptr_vo_bool = object.TryAs<runtime::ValueObject<bool>>();
        if (ptr_vo_bool != nullptr) {
            return ptr_vo_bool->GetValue();
        }

        return false;
    }

    void ClassInstance::Print(std::ostream& os, Context& context) {
        auto* met = cls_.GetMethod("__str__");
        if (met != nullptr) {
            Call(met->name, {}, context)->Print(os, context);
        }
        else {
            os << this;
        }
    }

    bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
        const Method* method_ = cls_.GetMethod(method);
        if (method_ != nullptr && method_->formal_params.size() == argument_count) {
            return true;
        }
        return false;
    }

    Closure& ClassInstance::Fields() {
        return closure_;
    }

    const Closure& ClassInstance::Fields() const {
        return closure_;
    }

    ClassInstance::ClassInstance(const Class& cls)
    : cls_(cls) {
    }

    ObjectHolder ClassInstance::Call(const std::string& method,
                                     const std::vector<ObjectHolder>& actual_args,
                                     Context& context) {
        if (!HasMethod(method, actual_args.size())) {
            throw std::runtime_error("Not implemented"s);
        }

        const Method* curr_method = cls_.GetMethod(method);
        Closure closure;
        closure["self"] = ObjectHolder::Share(*this);
        for (size_t i = 0; i < curr_method->formal_params.size(); ++i) {
            closure[curr_method->formal_params[i]] = actual_args[i];
        }

        return curr_method->body->Execute(closure, context);
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
    : parent_(parent), name_(name), methods_(std::move(methods)) {
        if (parent_ != nullptr) {
            std::for_each(parent_->methods_.begin(), parent_->methods_.end(), [this](const Method& meth) {methods_ptrs_[meth.name] = &meth; });
        }
        std::for_each(methods_.begin(), methods_.end(), [this](const Method& meth) {methods_ptrs_[meth.name] = &meth; });
    }

    const Method* Class::GetMethod(const std::string& name) const {
        if (methods_ptrs_.count(name) > 0) {
            return methods_ptrs_.at(name);
        }
        return nullptr;
    }

    const std::string& Class::GetName() const {
        return this->name_;
    }

    void Class::Print(ostream& os, Context& /*context*/) {
        os << "Class " << name_;
    }

    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        os << (GetValue() ? "True"sv : "False"sv);
    }

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (!lhs.operator bool() && !rhs.operator bool()) {
            return true;
        }
        auto obj1 = lhs.TryAs<String>();
        auto obj2 = rhs.TryAs<String>();
        if (obj1 != nullptr && obj2 != nullptr) {
            return obj1->GetValue() == obj2->GetValue();
        }
        auto obj1_bool = lhs.TryAs<Bool>();
        auto obj2_bool = rhs.TryAs<Bool>();
        if (obj1_bool != nullptr && obj2_bool != nullptr) {
            return obj1_bool->GetValue() == obj2_bool->GetValue();
        }
        auto obj1_Number = lhs.TryAs<Number>();
        auto obj2_Number = rhs.TryAs<Number>();
        if (obj1_Number != nullptr && obj2_Number != nullptr) {
            return obj1_Number->GetValue() == obj2_Number->GetValue();
        }
        auto obj1_class = lhs.TryAs<ClassInstance>();
        if (obj1_class != nullptr && rhs.TryAs<ClassInstance>() != nullptr) {
            if (obj1_class->HasMethod("__eq__", 1)) {
                return obj1_class->Call("__eq__", { rhs }, context).TryAs<Bool>()->GetValue();
            }
        }

        throw std::runtime_error("Cannot compare objects for equality"s);
    }

    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        auto obj1 = lhs.TryAs<String>();
        auto obj2 = rhs.TryAs<String>();
        if (obj1 != nullptr && obj2 != nullptr) {
            return obj1->GetValue() < obj2->GetValue();
        }
        auto obj1_bool = lhs.TryAs<Bool>();
        auto obj2_bool = rhs.TryAs<Bool>();
        if (obj1_bool != nullptr && obj2_bool != nullptr) {
            return obj1_bool->GetValue() < obj2_bool->GetValue();
        }
        auto obj1_Number = lhs.TryAs<Number>();
        auto obj2_Number = rhs.TryAs<Number>();
        if (obj1_Number != nullptr && obj2_Number != nullptr) {
            return obj1_Number->GetValue() < obj2_Number->GetValue();
        }
        auto obj1_class = lhs.TryAs<ClassInstance>();
        if (obj1_class != nullptr && rhs.TryAs<ClassInstance>() != nullptr) {
            if (obj1_class->HasMethod("__lt__", 1)) {
                return obj1_class->Call("__lt__", { rhs }, context).TryAs<Bool>()->GetValue();
            }
        }
        throw std::runtime_error("Cannot compare objects for less"s);
    }

    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Equal(lhs, rhs, context);
    }

    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
    }

    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
    }

    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context);
    }

}  // namespace runtime