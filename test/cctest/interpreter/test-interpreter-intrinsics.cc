// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#include "src/interpreter/interpreter-intrinsics.h"
#include "test/cctest/interpreter/interpreter-tester.h"

namespace v8 {
namespace internal {
namespace interpreter {

namespace {

class InvokeIntrinsicHelper {
 public:
  InvokeIntrinsicHelper(Isolate* isolate, Zone* zone,
                        Runtime::FunctionId function_id)
      : isolate_(isolate),
        zone_(zone),
        factory_(isolate->factory()),
        function_id_(function_id) {}

  template <class... A>
  Handle<Object> Invoke(A... args) {
    CHECK(IntrinsicsHelper::IsSupported(function_id_));
    BytecodeArrayBuilder builder(isolate_, zone_, sizeof...(args), 0, 0);
    builder.CallRuntime(function_id_, builder.Parameter(0), sizeof...(args))
        .Return();
    InterpreterTester tester(isolate_, builder.ToBytecodeArray());
    auto callable = tester.GetCallable<A...>();
    return callable(args...).ToHandleChecked();
  }

  Handle<Object> NewObject(const char* script) {
    return v8::Utils::OpenHandle(*CompileRun(script));
  }

  Handle<Object> Undefined() { return factory_->undefined_value(); }
  Handle<Object> Null() { return factory_->null_value(); }

 private:
  Isolate* isolate_;
  Zone* zone_;
  Factory* factory_;
  Runtime::FunctionId function_id_;
};

}  // namespace

TEST(IsJSReceiver) {
  HandleAndZoneScope handles;

  InvokeIntrinsicHelper helper(handles.main_isolate(), handles.main_zone(),
                               Runtime::kInlineIsJSReceiver);
  Factory* factory = handles.main_isolate()->factory();

  CHECK_EQ(*factory->true_value(),
           *helper.Invoke(helper.NewObject("new Date()")));
  CHECK_EQ(*factory->true_value(),
           *helper.Invoke(helper.NewObject("(function() {})")));
  CHECK_EQ(*factory->true_value(), *helper.Invoke(helper.NewObject("([1])")));
  CHECK_EQ(*factory->true_value(), *helper.Invoke(helper.NewObject("({})")));
  CHECK_EQ(*factory->true_value(), *helper.Invoke(helper.NewObject("(/x/)")));
  CHECK_EQ(*factory->false_value(), *helper.Invoke(helper.Undefined()));
  CHECK_EQ(*factory->false_value(), *helper.Invoke(helper.Null()));
  CHECK_EQ(*factory->false_value(),
           *helper.Invoke(helper.NewObject("'string'")));
  CHECK_EQ(*factory->false_value(), *helper.Invoke(helper.NewObject("42")));
}

TEST(IsArray) {
  HandleAndZoneScope handles;

  InvokeIntrinsicHelper helper(handles.main_isolate(), handles.main_zone(),
                               Runtime::kInlineIsArray);
  Factory* factory = handles.main_isolate()->factory();

  CHECK_EQ(*factory->false_value(),
           *helper.Invoke(helper.NewObject("new Date()")));
  CHECK_EQ(*factory->false_value(),
           *helper.Invoke(helper.NewObject("(function() {})")));
  CHECK_EQ(*factory->true_value(), *helper.Invoke(helper.NewObject("([1])")));
  CHECK_EQ(*factory->false_value(), *helper.Invoke(helper.NewObject("({})")));
  CHECK_EQ(*factory->false_value(), *helper.Invoke(helper.NewObject("(/x/)")));
  CHECK_EQ(*factory->false_value(), *helper.Invoke(helper.Undefined()));
  CHECK_EQ(*factory->false_value(), *helper.Invoke(helper.Null()));
  CHECK_EQ(*factory->false_value(),
           *helper.Invoke(helper.NewObject("'string'")));
  CHECK_EQ(*factory->false_value(), *helper.Invoke(helper.NewObject("42")));
}

TEST(Call) {
  HandleAndZoneScope handles;
  Isolate* isolate = handles.main_isolate();
  Factory* factory = isolate->factory();
  InvokeIntrinsicHelper helper(isolate, handles.main_zone(),
                               Runtime::kInlineCall);

  CHECK_EQ(Smi::FromInt(20),
           *helper.Invoke(helper.NewObject("(function() { return this.x; })"),
                          helper.NewObject("({ x: 20 })")));
  CHECK_EQ(Smi::FromInt(50),
           *helper.Invoke(helper.NewObject("(function(arg1) { return arg1; })"),
                          factory->undefined_value(),
                          handle(Smi::FromInt(50), isolate)));
  CHECK_EQ(
      Smi::FromInt(20),
      *helper.Invoke(
          helper.NewObject("(function(a, b, c) { return a + b + c; })"),
          factory->undefined_value(), handle(Smi::FromInt(10), isolate),
          handle(Smi::FromInt(7), isolate), handle(Smi::FromInt(3), isolate)));
}

}  // namespace interpreter
}  // namespace internal
}  // namespace v8
