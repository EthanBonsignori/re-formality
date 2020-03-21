module Debouncer = Formality__Debouncer;
module ReactUpdate = Formality__ReactUpdate;

type strategy =
  | OnFirstBlur
  | OnFirstChange
  | OnFirstSuccess
  | OnFirstSuccessOrFirstBlur
  | OnSubmit;

type visibility =
  | Shown
  | Hidden;

type fieldStatus('outputValue, 'message) =
  | Pristine
  | Dirty(result('outputValue, 'message), visibility);

type collectionStatus('message) = result(unit, 'message);

type formStatus('submissionError) =
  | Editing
  | Submitting(option('submissionError))
  | Submitted
  | SubmissionFailed('submissionError);

type submissionStatus =
  | NeverSubmitted
  | AttemptedToSubmit;

let exposeFieldResult =
    (fieldStatus: fieldStatus('outputValue, 'message))
    : option(result('outputValue, 'message)) =>
  switch (fieldStatus) {
  | Pristine
  | Dirty(_, Hidden) => None
  | Dirty(result, Shown) => Some(result)
  };

type index = int;

type singleValueValidator('input, 'outputValue, 'message) = {
  strategy,
  validate: 'input => result('outputValue, 'message),
};

type collectionValidatorWithWholeCollectionValidator(
  'input,
  'message,
  'fieldsValidators,
) = {
  collection: 'input => result(unit, 'message),
  fields: 'fieldsValidators,
};

type collectionValidatorWithoutWholeCollectionValidator('fieldsValidators) = {
  collection: unit,
  fields: 'fieldsValidators,
};

type valueOfCollectionValidator('input, 'outputValue, 'message) = {
  strategy,
  validate: ('input, ~at: index) => result('outputValue, 'message),
};

type formValidationResult('output, 'fieldsStatuses, 'collectionsStatuses) =
  | Valid({
      output: 'output,
      fieldsStatuses: 'fieldsStatuses,
      collectionsStatuses: 'collectionsStatuses,
    })
  | Invalid({
      fieldsStatuses: 'fieldsStatuses,
      collectionsStatuses: 'collectionsStatuses,
    });

type submissionCallbacks('input, 'submissionError) = {
  notifyOnSuccess: option('input) => unit,
  notifyOnFailure: 'submissionError => unit,
  reset: unit => unit,
  dismissSubmissionResult: unit => unit,
};

let validateFieldOnChangeWithoutValidator =
    (
      ~fieldInput: 'outputValue,
      ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
    )
    : 'statuses => {
  Dirty(Ok(fieldInput), Hidden)->setStatus;
};

let validateFieldOnChangeWithValidator =
    (
      ~input: 'input,
      ~fieldStatus: fieldStatus('outputValue, 'message),
      ~submissionStatus: submissionStatus,
      ~validator: singleValueValidator('input, 'outputValue, 'message),
      ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
    )
    : 'statuses => {
  switch (validator.strategy, fieldStatus, submissionStatus) {
  | (_, Dirty(_, Shown), _)
  | (_, _, AttemptedToSubmit)
  | (OnFirstChange, _, NeverSubmitted) =>
    Dirty(validator.validate(input), Shown)->setStatus
  | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
    switch (validator.validate(input)) {
    | Ok(_) as result => Dirty(result, Shown)->setStatus
    | Error(_) as result => Dirty(result, Hidden)->setStatus
    }
  | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
    Dirty(validator.validate(input), Hidden)->setStatus
  };
};

let validateFieldOfCollectionOnChangeWithValidator =
    (
      ~input: 'input,
      ~index: index,
      ~fieldStatus: fieldStatus('outputValue, 'message),
      ~submissionStatus: submissionStatus,
      ~validator: valueOfCollectionValidator('input, 'outputValue, 'message),
      ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
    )
    : 'statuses => {
  switch (validator.strategy, fieldStatus, submissionStatus) {
  | (_, Dirty(_, Shown), _)
  | (_, _, AttemptedToSubmit)
  | (OnFirstChange, _, NeverSubmitted) =>
    Dirty(validator.validate(input, ~at=index), Shown)->setStatus
  | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
    switch (validator.validate(input, ~at=index)) {
    | Ok(_) as result => Dirty(result, Shown)->setStatus
    | Error(_) as result => Dirty(result, Hidden)->setStatus
    }
  | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
    Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
  };
};

let validateDependentFieldOnChange =
    (
      ~input: 'input,
      ~fieldStatus: fieldStatus('outputValue, 'message),
      ~validator: singleValueValidator('input, 'outputValue, 'message),
      ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
    )
    : option('statuses) => {
  switch (fieldStatus) {
  | Pristine
  | Dirty(_, Hidden) => None
  | Dirty(_, Shown) =>
    Dirty(validator.validate(input), Shown)->setStatus->Some
  };
};

let validateDependentFieldOfCollectionOnChange =
    (
      ~input: 'input,
      ~index: index,
      ~fieldStatus: fieldStatus('outputValue, 'message),
      ~validator: valueOfCollectionValidator('input, 'outputValue, 'message),
      ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
    )
    : option('statuses) => {
  switch (fieldStatus) {
  | Pristine
  | Dirty(_, Hidden) => None
  | Dirty(_, Shown) =>
    Dirty(validator.validate(input, ~at=index), Shown)->setStatus->Some
  };
};

let validateFieldOnBlurWithoutValidator =
    (
      ~fieldInput: 'outputValue,
      ~fieldStatus: fieldStatus('outputValue, 'message),
      ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
    )
    : option('statuses) =>
  switch (fieldStatus) {
  | Dirty(_, Shown | Hidden) => None
  | Pristine => Dirty(Ok(fieldInput), Hidden)->setStatus->Some
  };

let validateFieldOnBlurWithValidator =
    (
      ~input: 'input,
      ~fieldStatus: fieldStatus('outputValue, 'message),
      ~validator: singleValueValidator('input, 'outputValue, 'message),
      ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
    )
    : option('statuses) => {
  switch (fieldStatus) {
  | Dirty(_, Shown) => None
  | Pristine
  | Dirty(_, Hidden) =>
    switch (validator.strategy) {
    | OnFirstChange
    | OnFirstSuccess
    | OnSubmit => Dirty(validator.validate(input), Hidden)->setStatus->Some
    | OnFirstBlur
    | OnFirstSuccessOrFirstBlur =>
      Dirty(validator.validate(input), Shown)->setStatus->Some
    }
  };
};

let validateFieldOfCollectionOnBlurWithValidator =
    (
      ~input: 'input,
      ~index: index,
      ~fieldStatus: fieldStatus('outputValue, 'message),
      ~validator: valueOfCollectionValidator('input, 'outputValue, 'message),
      ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
    )
    : option('statuses) => {
  switch (fieldStatus) {
  | Dirty(_, Shown) => None
  | Pristine
  | Dirty(_, Hidden) =>
    switch (validator.strategy) {
    | OnFirstChange
    | OnFirstSuccess
    | OnSubmit =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus->Some
    | OnFirstBlur
    | OnFirstSuccessOrFirstBlur =>
      Dirty(validator.validate(input, ~at=index), Shown)->setStatus->Some
    }
  };
};

module Async = {
  type fieldStatus('outputValue, 'message) =
    | Pristine
    | Dirty(result('outputValue, 'message), visibility)
    | Validating('outputValue);

  type exposedFieldStatus('outputValue, 'message) =
    | Validating('outputValue)
    | Result(result('outputValue, 'message));

  type singleValueValidator('input, 'outputValue, 'message, 'action) = {
    strategy,
    validate: 'input => result('outputValue, 'message),
    validateAsync: (('outputValue, 'action => unit)) => unit,
    eq: ('outputValue, 'outputValue) => bool,
  };

  type valueOfCollectionValidator('input, 'outputValue, 'message, 'action) = {
    strategy,
    validate: ('input, ~at: index) => result('outputValue, 'message),
    validateAsync: (('outputValue, index, 'action => unit)) => unit,
    eq: ('outputValue, 'outputValue) => bool,
  };

  type validateAsyncFn('outputValue, 'message) =
    'outputValue => Js.Promise.t(result('outputValue, 'message));

  let validateAsync =
      (
        ~value: 'outputValue,
        ~validate: validateAsyncFn('outputValue, 'message),
        ~andThen: result('outputValue, 'message) => unit,
      )
      : unit =>
    validate(value)
    ->Js.Promise.(then_(res => res->andThen->resolve, _))
    ->ignore;

  type formValidationResult('output, 'fieldsStatuses, 'collectionsStatuses) =
    | Valid({
        output: 'output,
        fieldsStatuses: 'fieldsStatuses,
        collectionsStatuses: 'collectionsStatuses,
      })
    | Invalid({
        fieldsStatuses: 'fieldsStatuses,
        collectionsStatuses: 'collectionsStatuses,
      })
    | Validating({
        fieldsStatuses: 'fieldsStatuses,
        collectionsStatuses: 'collectionsStatuses,
      });

  let exposeFieldResult =
      (fieldStatus: fieldStatus('outputValue, 'message))
      : option(exposedFieldStatus('outputValue, 'message)) =>
    switch (fieldStatus) {
    | Pristine
    | Dirty(_, Hidden) => None
    | Validating(x) => Some(Validating(x))
    | Dirty(result, Shown) => Some(Result(result))
    };

  let validateFieldOnChangeInOnBlurMode =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           singleValueValidator('input, 'outputValue, 'message, 'action),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(_) as result => Dirty(result, Hidden)->setStatus
      | Error(_) as result => Dirty(result, Shown)->setStatus
      }
    | (
        OnFirstSuccess | OnFirstSuccessOrFirstBlur | OnFirstBlur | OnSubmit,
        _,
        NeverSubmitted,
      ) =>
      Dirty(validator.validate(input), Hidden)->setStatus
    };
  };

  let validateFieldOfCollectionOnChangeInOnBlurMode =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           valueOfCollectionValidator(
             'input,
             'outputValue,
             'message,
             'action,
           ),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(_) as result => Dirty(result, Hidden)->setStatus
      | Error(_) as result => Dirty(result, Shown)->setStatus
      }
    | (
        OnFirstSuccess | OnFirstSuccessOrFirstBlur | OnFirstBlur | OnSubmit,
        _,
        NeverSubmitted,
      ) =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
    };
  };

  let validateFieldOfOptionTypeOnChangeInOnBlurMode =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           singleValueValidator('input, 'outputValue, 'message, 'action),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(Some(_)) as result => Dirty(result, Hidden)->setStatus
      | Ok(None) as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(None) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_)) as result
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input), Hidden)->setStatus
    };
  };

  let validateFieldOfCollectionOfOptionTypeOnChangeInOnBlurMode =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           valueOfCollectionValidator(
             'input,
             'outputValue,
             'message,
             'action,
           ),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(Some(_)) as result => Dirty(result, Hidden)->setStatus
      | Ok(None) as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(None) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_)) as result
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
    };
  };

  let validateFieldOfStringTypeOnChangeInOnBlurMode =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus(string, 'message),
        ~submissionStatus: submissionStatus,
        ~validator: singleValueValidator('input, string, 'message, 'action),
        ~setStatus: fieldStatus(string, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok("") as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      | Ok(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok("") as result => Dirty(result, Shown)->setStatus
      | Ok(_) as result
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input), Hidden)->setStatus
    };
  };

  let validateFieldOfCollectionOfStringTypeOnChangeInOnBlurMode =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus(string, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           valueOfCollectionValidator('input, string, 'message, 'action),
        ~setStatus: fieldStatus(string, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok("") as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      | Ok(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok("") as result => Dirty(result, Shown)->setStatus
      | Ok(_) as result
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
    };
  };

  let validateFieldOfOptionStringTypeOnChangeInOnBlurMode =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus(option(string), 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           singleValueValidator('input, option(string), 'message, 'action),
        ~setStatus: fieldStatus(option(string), 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(Some("")) as result
      | Ok(None) as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_)) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(Some("")) as result
      | Ok(None) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_)) as result
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input), Hidden)->setStatus
    };
  };

  let validateFieldOfCollectionOfOptionStringTypeOnChangeInOnBlurMode =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus(option(string), 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           valueOfCollectionValidator(
             'input,
             option(string),
             'message,
             'action,
           ),
        ~setStatus: fieldStatus(option(string), 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(Some("")) as result
      | Ok(None) as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_)) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(Some("")) as result
      | Ok(None) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_)) as result
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
    };
  };

  let validateFieldOnChangeInOnChangeMode =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           singleValueValidator('input, 'outputValue, 'message, 'action),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(x) => Validating(x)->setStatus
      | Error(_) as result => Dirty(result, Shown)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(x) => Validating(x)->setStatus
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input), Hidden)->setStatus
    };
  };

  let validateFieldOfCollectionOnChangeInOnChangeMode =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           valueOfCollectionValidator(
             'input,
             'outputValue,
             'message,
             'action,
           ),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(x) => Validating(x)->setStatus
      | Error(_) as result => Dirty(result, Shown)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(x) => Validating(x)->setStatus
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
    };
  };

  let validateFieldOfOptionTypeOnChangeInOnChangeMode =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           singleValueValidator('input, 'outputValue, 'message, 'action),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(Some(_) as x) => Validating(x)->setStatus
      | Ok(None) as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(Some(_) as x) => Validating(x)->setStatus
      | Ok(None) as result => Dirty(result, Shown)->setStatus
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input), Hidden)->setStatus
    };
  };

  let validateFieldOfCollectionOfOptionTypeOnChangeInOnChangeMode =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           valueOfCollectionValidator(
             'input,
             'outputValue,
             'message,
             'action,
           ),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(Some(_) as x) => Validating(x)->setStatus
      | Ok(None) as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(Some(_) as x) => Validating(x)->setStatus
      | Ok(None) as result => Dirty(result, Shown)->setStatus
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
    };
  };

  let validateFieldOfStringTypeOnChangeInOnChangeMode =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus(string, 'message),
        ~submissionStatus: submissionStatus,
        ~validator: singleValueValidator('input, string, 'message, 'action),
        ~setStatus: fieldStatus(string, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok("") as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      | Ok(x) => Validating(x)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok("") as result => Dirty(result, Shown)->setStatus
      | Ok(x) => Validating(x)->setStatus
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input), Hidden)->setStatus
    };
  };

  let validateFieldOfCollectionOfStringTypeOnChangeInOnChangeMode =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus(string, 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           valueOfCollectionValidator('input, string, 'message, 'action),
        ~setStatus: fieldStatus(string, 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok("") as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      | Ok(x) => Validating(x)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok("") as result => Dirty(result, Shown)->setStatus
      | Ok(x) => Validating(x)->setStatus
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
    };
  };

  let validateFieldOfOptionStringTypeOnChangeInOnChangeMode =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus(option(string), 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           singleValueValidator('input, option(string), 'message, 'action),
        ~setStatus: fieldStatus(option(string), 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(Some("")) as result
      | Ok(None) as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_) as x) => Validating(x)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input)) {
      | Ok(Some("")) as result
      | Ok(None) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_) as x) => Validating(x)->setStatus
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input), Hidden)->setStatus
    };
  };

  let validateFieldOfCollectionOfOptionStringTypeOnChangeInOnChangeMode =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus(option(string), 'message),
        ~submissionStatus: submissionStatus,
        ~validator:
           valueOfCollectionValidator(
             'input,
             option(string),
             'message,
             'action,
           ),
        ~setStatus: fieldStatus(option(string), 'message) => 'statuses,
      )
      : 'statuses => {
    switch (validator.strategy, fieldStatus, submissionStatus) {
    | (_, Dirty(_, Shown), _)
    | (_, _, AttemptedToSubmit)
    | (OnFirstChange, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(Some("")) as result
      | Ok(None) as result
      | Error(_) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_) as x) => Validating(x)->setStatus
      }
    | (OnFirstSuccess | OnFirstSuccessOrFirstBlur, _, NeverSubmitted) =>
      switch (validator.validate(input, ~at=index)) {
      | Ok(Some("")) as result
      | Ok(None) as result => Dirty(result, Shown)->setStatus
      | Ok(Some(_) as x) => Validating(x)->setStatus
      | Error(_) as result => Dirty(result, Hidden)->setStatus
      }
    | (OnFirstBlur | OnSubmit, _, NeverSubmitted) =>
      Dirty(validator.validate(input, ~at=index), Hidden)->setStatus
    };
  };

  let validateDependentFieldOnChange =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~validator:
           singleValueValidator('input, 'outputValue, 'message, 'action),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Pristine
    | Validating(_)
    | Dirty(_, Hidden) => None
    | Dirty(_, Shown) =>
      Dirty(validator.validate(input), Shown)->setStatus->Some
    };
  };

  let validateDependentFieldOfCollectionOnChange =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~validator:
           valueOfCollectionValidator(
             'input,
             'outputValue,
             'message,
             'action,
           ),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Pristine
    | Validating(_)
    | Dirty(_, Hidden) => None
    | Dirty(_, Shown) =>
      Dirty(validator.validate(input, ~at=index), Shown)->setStatus->Some
    };
  };

  let validateFieldOnBlur =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~validator:
           singleValueValidator('input, 'outputValue, 'message, 'action),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Validating(_)
    | Dirty(_, Shown) => None
    | Pristine
    | Dirty(_, Hidden) =>
      switch (validator.strategy) {
      | OnFirstChange
      | OnFirstSuccess
      | OnSubmit => Dirty(validator.validate(input), Hidden)->setStatus->Some
      | OnFirstBlur
      | OnFirstSuccessOrFirstBlur =>
        switch (validator.validate(input)) {
        | Ok(x) => Validating(x)->setStatus->Some
        | Error(_) as result => Dirty(result, Shown)->setStatus->Some
        }
      }
    };
  };

  let validateFieldOfCollectionOnBlur =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~validator:
           valueOfCollectionValidator(
             'input,
             'outputValue,
             'message,
             'action,
           ),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Validating(_)
    | Dirty(_, Shown) => None
    | Pristine
    | Dirty(_, Hidden) =>
      switch (validator.strategy) {
      | OnFirstChange
      | OnFirstSuccess
      | OnSubmit =>
        Dirty(validator.validate(input, ~at=index), Hidden)->setStatus->Some
      | OnFirstBlur
      | OnFirstSuccessOrFirstBlur =>
        switch (validator.validate(input, ~at=index)) {
        | Ok(x) => Validating(x)->setStatus->Some
        | Error(_) as result => Dirty(result, Shown)->setStatus->Some
        }
      }
    };
  };

  let validateFieldOfOptionTypeOnBlur =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~validator:
           singleValueValidator('input, 'outputValue, 'message, 'action),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Validating(_)
    | Dirty(_, Shown) => None
    | Pristine
    | Dirty(_, Hidden) =>
      switch (validator.strategy) {
      | OnFirstChange
      | OnFirstSuccess
      | OnSubmit => Dirty(validator.validate(input), Hidden)->setStatus->Some
      | OnFirstBlur
      | OnFirstSuccessOrFirstBlur =>
        switch (validator.validate(input)) {
        | Ok(Some(_) as x) => Validating(x)->setStatus->Some
        | Ok(None) as result
        | Error(_) as result => Dirty(result, Shown)->setStatus->Some
        }
      }
    };
  };

  let validateFieldOfCollectionOfOptionTypeOnBlur =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus('outputValue, 'message),
        ~validator:
           valueOfCollectionValidator(
             'input,
             'outputValue,
             'message,
             'action,
           ),
        ~setStatus: fieldStatus('outputValue, 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Validating(_)
    | Dirty(_, Shown) => None
    | Pristine
    | Dirty(_, Hidden) =>
      switch (validator.strategy) {
      | OnFirstChange
      | OnFirstSuccess
      | OnSubmit =>
        Dirty(validator.validate(input, ~at=index), Hidden)->setStatus->Some
      | OnFirstBlur
      | OnFirstSuccessOrFirstBlur =>
        switch (validator.validate(input, ~at=index)) {
        | Ok(Some(_) as x) => Validating(x)->setStatus->Some
        | Ok(None) as result
        | Error(_) as result => Dirty(result, Shown)->setStatus->Some
        }
      }
    };
  };

  let validateFieldOfStringTypeOnBlur =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus(string, 'message),
        ~validator: singleValueValidator('input, string, 'message, 'action),
        ~setStatus: fieldStatus(string, 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Validating(_)
    | Dirty(_, Shown) => None
    | Pristine
    | Dirty(_, Hidden) =>
      switch (validator.strategy) {
      | OnFirstChange
      | OnFirstSuccess
      | OnSubmit => Dirty(validator.validate(input), Hidden)->setStatus->Some
      | OnFirstBlur
      | OnFirstSuccessOrFirstBlur =>
        switch (validator.validate(input)) {
        | Ok("") as result
        | Error(_) as result => Dirty(result, Shown)->setStatus->Some
        | Ok(x) => Validating(x)->setStatus->Some
        }
      }
    };
  };

  let validateFieldOfCollectionOfStringTypeOnBlur =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus(string, 'message),
        ~validator:
           valueOfCollectionValidator('input, string, 'message, 'action),
        ~setStatus: fieldStatus(string, 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Validating(_)
    | Dirty(_, Shown) => None
    | Pristine
    | Dirty(_, Hidden) =>
      switch (validator.strategy) {
      | OnFirstChange
      | OnFirstSuccess
      | OnSubmit =>
        Dirty(validator.validate(input, ~at=index), Hidden)->setStatus->Some
      | OnFirstBlur
      | OnFirstSuccessOrFirstBlur =>
        switch (validator.validate(input, ~at=index)) {
        | Ok("") as result
        | Error(_) as result => Dirty(result, Shown)->setStatus->Some
        | Ok(x) => Validating(x)->setStatus->Some
        }
      }
    };
  };

  let validateFieldOfOptionStringTypeOnBlur =
      (
        ~input: 'input,
        ~fieldStatus: fieldStatus(option(string), 'message),
        ~validator:
           singleValueValidator('input, option(string), 'message, 'action),
        ~setStatus: fieldStatus(option(string), 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Validating(_)
    | Dirty(_, Shown) => None
    | Pristine
    | Dirty(_, Hidden) =>
      switch (validator.strategy) {
      | OnFirstChange
      | OnFirstSuccess
      | OnSubmit => Dirty(validator.validate(input), Hidden)->setStatus->Some
      | OnFirstBlur
      | OnFirstSuccessOrFirstBlur =>
        switch (validator.validate(input)) {
        | Ok(Some("")) as result
        | Ok(None) as result
        | Error(_) as result => Dirty(result, Shown)->setStatus->Some
        | Ok(Some(_) as x) => Validating(x)->setStatus->Some
        }
      }
    };
  };

  let validateFieldOfCollectionOfOptionStringTypeOnBlur =
      (
        ~input: 'input,
        ~index: index,
        ~fieldStatus: fieldStatus(option(string), 'message),
        ~validator:
           valueOfCollectionValidator(
             'input,
             option(string),
             'message,
             'action,
           ),
        ~setStatus: fieldStatus(option(string), 'message) => 'statuses,
      )
      : option('statuses) => {
    switch (fieldStatus) {
    | Validating(_)
    | Dirty(_, Shown) => None
    | Pristine
    | Dirty(_, Hidden) =>
      switch (validator.strategy) {
      | OnFirstChange
      | OnFirstSuccess
      | OnSubmit =>
        Dirty(validator.validate(input, ~at=index), Hidden)->setStatus->Some
      | OnFirstBlur
      | OnFirstSuccessOrFirstBlur =>
        switch (validator.validate(input, ~at=index)) {
        | Ok(Some("")) as result
        | Ok(None) as result
        | Error(_) as result => Dirty(result, Shown)->setStatus->Some
        | Ok(Some(_) as x) => Validating(x)->setStatus->Some
        }
      }
    };
  };
};
