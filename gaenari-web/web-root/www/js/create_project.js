
// global
var index = 0;

function add_attribute() {
    var name = $('#attribute-name').val();
    var data_type_id = $('#attribute-data-type').children(':selected').attr('id');
    var selection_id = $('#attribute-selection').children(':selected').attr('id');
    var data_type_text = $('#attribute-data-type').children(':selected').val();
    var selection_text = $('#attribute-selection').children(':selected').val();
    var html = $('#attribute-list').html();
    var node = 
`<tr id="attribute-index${index}">
    <td><input type="hidden" value="${name}/${data_type_id}/${selection_id}" name="index${index}" />${name}</td>
    <td>${data_type_text}</td>
    <td>${selection_text}</td>
    <td class="text-right py-0 align-middle">
        <div class="btn-group btn-group-sm">
            <a href="javascript:void(0);" onclick="remove_attribute('attribute-index${index}')" class="btn btn-danger"><i class="fas fa-minus"></i></a>
        </div>
    </td>
</tr>`;

    html += node;
    $('#attribute-list').html(html);

    index++;
}

function remove_attribute(id) {
    $(`#${id}`).remove();
}

// form validator.
$(function () {
  $.validator.setDefaults({
    submitHandler: function () {
    }
  });
  $('#create_project_form').validate({
    rules: {
      project_name: {
        required: true,
      },
    },
    messages: {
      project_name: {
        required: "Please enter a project name.",
       },
    },
    errorElement: 'span',
    errorPlacement: function (error, element) {
      error.addClass('invalid-feedback');
      element.closest('.form-group').append(error);
    },
    highlight: function (element, errorClass, validClass) {
      $(element).addClass('is-invalid');
    },
    unhighlight: function (element, errorClass, validClass) {
      $(element).removeClass('is-invalid');
    }
  });
});


/*
$(function () {
  $.validator.setDefaults({
    submitHandler: function () {
      alert( "Form successful submitted!" );
    }
  });
  $('#create_project_form').validate({
    rules: {
      email: {
        required: true,
        email: true,
      },
      password: {
        required: true,
        minlength: 5
      },
      terms: {
        required: true
      },
    },
    messages: {
      email: {
        required: "Please enter a email address",
        email: "Please enter a valid email address"
      },
      password: {
        required: "Please provide a password",
        minlength: "Your password must be at least 5 characters long"
      },
      terms: "Please accept our terms"
    },
    errorElement: 'span',
    errorPlacement: function (error, element) {
      error.addClass('invalid-feedback');
      element.closest('.form-group').append(error);
    },
    highlight: function (element, errorClass, validClass) {
      $(element).addClass('is-invalid');
    },
    unhighlight: function (element, errorClass, validClass) {
      $(element).removeClass('is-invalid');
    }
  });
});
*/
