
open Parsetree;

open Codemod.Helpers;
Printexc.record_backtrace(true);
// Util.Log.spamError := true;
// References.debugReferences := true;
// MerlinFile.debug := true;
print_endline("Spamming error");

/*

"Convert all Error(x) to Error(Unspecified(x)) if the function's return type is Belt.Result.t(int, string)"

 */

let replaceErrors = (ctx, expr) =>
  expr
  ->mapExpr((_mapper, expr) => {
      switch (expr.pexp_desc) {
        /* We should be more flexible here, e.g. to accept Result.Error() in addition to Error() */
      | Pexp_construct({txt: Longident.Lident("Error")}, Some({pexp_desc: Pexp_tuple([arg])})) =>
        let loc = Location.none
        switch (ctx->getExprType(arg)) {
          | Reference(Builtin("string"), []) => Some([%expr Error(Unspecified([%e arg]))])
          | _ => None
        }
      | _ => None
      };
    });

let modify = (ctx, structure) => {
  print_endline("ok");
  structure->strExpr((_mapper, expr) =>
      expr->mapFnExpr((_mapper, args, body) => {
          switch (ctx->getExprType(body)) {
            /* The type Belt.Result.t is just an alias for Belt_Result.t, and we have to specify the "original declaration" path */
          | Reference(Public({moduleName: "Lib__Util", modulePath: ["Belt_Result"], name: "t"}), [Reference(Builtin("int"), []), Reference(Builtin("string"), [])]) =>
            Some((args, ctx->replaceErrors(body)))
          // | Reference(Public({moduleName: something, modulePath: ["Belt_Result"], name: "t"}), [Reference(Builtin("int"), []), Reference(Builtin("string"), [])]) =>
          //   print_endline(something);
          //   None
          | _ =>
            None
          };
        })
      ->Some
    );
};

switch (Sys.argv) {
  | [|_, root|] =>
    print_endline("Running on project: " ++ root);
    Codemod.run(
      ~rootPath=root,
      ~filterPath=(path, _moduleName) => 
      // Str.string_match(
      //   Str.regexp(".*/Example.re$"),
      //   path,
      //   0
      // ),
      Filename.extension(path) == ".re",
      modify
    );
  | _ => ()
};
