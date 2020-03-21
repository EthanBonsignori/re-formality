open Meta;
open Ast;
open AstHelpers;

open Ppxlib;
open Ast_helper;

let ast =
    (
      ~loc,
      ~field: Scheme.field,
      ~collection: Collection.t,
      ~optionality: option(FieldOptionality.t),
      ~field_status_expr: expression,
      ~validator_expr: expression,
      ~set_status_expr: expression,
    ) => {
  %expr
  {
    let result =
      switch%e (optionality) {
      | None =>
        %expr
        {
          Async.validateFieldOfCollectionOnBlur(
            ~input=state.input,
            ~index,
            ~fieldStatus=[%e field_status_expr],
            ~validator=[%e validator_expr],
            ~setStatus=[%e [%expr status => [%e set_status_expr]]],
          );
        }
      | Some(OptionType) =>
        %expr
        {
          Async.validateFieldOfCollectionOfOptionTypeOnBlur(
            ~input=state.input,
            ~index,
            ~fieldStatus=[%e field_status_expr],
            ~validator=[%e validator_expr],
            ~setStatus=[%e [%expr status => [%e set_status_expr]]],
          );
        }
      | Some(StringType) =>
        %expr
        {
          Async.validateFieldOfCollectionOfStringTypeOnBlur(
            ~input=state.input,
            ~index,
            ~fieldStatus=[%e field_status_expr],
            ~validator=[%e validator_expr],
            ~setStatus=[%e [%expr status => [%e set_status_expr]]],
          );
        }
      | Some(OptionStringType) =>
        %expr
        {
          Async.validateFieldOfCollectionOfOptionStringTypeOnBlur(
            ~input=state.input,
            ~index,
            ~fieldStatus=[%e field_status_expr],
            ~validator=[%e validator_expr],
            ~setStatus=[%e [%expr status => [%e set_status_expr]]],
          );
        }
      };

    switch (result) {
    | None => NoUpdate
    | Some(fieldsStatuses) =>
      switch (
        [%e
          field.name
          |> E.field_of_collection(~in_="fieldsStatuses", ~collection, ~loc)
        ]
      ) {
      | Validating(value) =>
        UpdateWithSideEffects(
          {...state, fieldsStatuses},
          ({state: _, dispatch}) => {
            %e
            E.apply_field4(
              ~in_=("validators", collection.plural, "fields", field.name),
              ~fn="validateAsync",
              ~args=[(Nolabel, [%expr (value, index, dispatch)])],
              ~loc,
            )
          },
        )
      | Pristine
      | Dirty(_, Shown | Hidden) => Update({...state, fieldsStatuses})
      }
    };
  };
};
