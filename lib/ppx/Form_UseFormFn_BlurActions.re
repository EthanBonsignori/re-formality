open Meta;
open Ast;
open AstHelpers;
open Printer;

open Ppxlib;
open Ast_helper;

let ast = (~loc, scheme: Scheme.t) =>
  scheme
  |> List.fold_left(
       (acc, entry: Scheme.entry) =>
         switch (entry) {
         | Field(field) => [
             Exp.case(
               Pat.construct(
                 Lident(FieldPrinter.blur_action(~field=field.name))
                 |> lid(~loc),
                 None,
               ),
               {
                 let field_status_expr =
                   field.name
                   |> E.field2(~in_=("state", "fieldsStatuses"), ~loc);
                 let field_input_expr =
                   field.name |> E.field2(~in_=("state", "input"), ~loc);
                 let validator_expr =
                   field.name |> E.field(~in_="validators", ~loc);
                 let set_status_expr =
                   field.name
                   |> E.update_field2(
                        ~in_=("state", "fieldsStatuses"),
                        ~with_=[%expr status],
                        ~loc,
                      );

                 switch (field.validator) {
                 | SyncValidator(validator) =>
                   Form_UseFormFn_BlurActions_SyncField.ast(
                     ~loc,
                     ~validator,
                     ~field_status_expr,
                     ~field_input_expr,
                     ~validator_expr,
                     ~set_status_expr,
                   )
                 | AsyncValidator({optionality}) =>
                   Form_UseFormFn_BlurActions_AsyncField.ast(
                     ~loc,
                     ~field,
                     ~optionality,
                     ~field_status_expr,
                     ~validator_expr,
                     ~set_status_expr,
                   )
                 };
               },
             ),
             ...acc,
           ]
         | Collection({collection, fields}) =>
           fields
           |> List.fold_left(
                (acc, field: Scheme.field) =>
                  [
                    Exp.case(
                      Pat.construct(
                        ~attrs=[explicit_arity(~loc)],
                        Lident(
                          FieldOfCollectionPrinter.blur_action(
                            ~collection,
                            ~field=field.name,
                          ),
                        )
                        |> lid(~loc),
                        Some(Pat.tuple([Pat.var("index" |> str(~loc))])),
                      ),
                      {
                        let field_status_expr =
                          field.name
                          |> E.field_of_collection2(
                               ~in_=("state", "fieldsStatuses"),
                               ~collection,
                               ~loc,
                             );
                        let field_input_expr =
                          field.name
                          |> E.field_of_collection2(
                               ~in_=("state", "input"),
                               ~collection,
                               ~loc,
                             );
                        let validator_expr =
                          field.name
                          |> E.field_of_collection_validator(
                               ~validators="validators",
                               ~collection,
                               ~loc,
                             );
                        let set_status_expr =
                          field.name
                          |> E.update_field_of_collection2(
                               ~in_=("state", "fieldsStatuses"),
                               ~collection,
                               ~with_=[%expr status],
                               ~loc,
                             );

                        switch (field.validator) {
                        | SyncValidator(validator) =>
                          Form_UseFormFn_BlurActions_SyncFieldOfCollection.ast(
                            ~loc,
                            ~validator,
                            ~field_status_expr,
                            ~field_input_expr,
                            ~validator_expr,
                            ~set_status_expr,
                          )
                        | AsyncValidator({optionality}) =>
                          Form_UseFormFn_BlurActions_AsyncFieldOfCollection.ast(
                            ~loc,
                            ~field,
                            ~collection,
                            ~optionality,
                            ~field_status_expr,
                            ~validator_expr,
                            ~set_status_expr,
                          )
                        };
                      },
                    ),
                    ...acc,
                  ],
                acc,
              )
         },
       [],
     );
