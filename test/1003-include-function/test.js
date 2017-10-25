
test_expect(include("include-empty.js") === undefined);
test_expect(include("include-ok1.js") === "OK");
test_expect_except(include("include-parse-error.js"));
test_expect_except(include("include-runtime-error1.js"));
test_expect_except(include("include-runtime-error2.js"));

