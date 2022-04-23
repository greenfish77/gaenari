
// global
var index = 0;
var count_x = 0;
var count_y = 0;
var attribute_names = {};

function on_submit() {
    // validate before submit.
    var toast_msg;
    if (count_x <= 2) toast_msg = 'Minimum 3 or more X required.';
    if (count_y != 1) toast_msg = 'One Y required.';
    if (toast_msg) {
        $(document).Toasts('create', {
            title: 'Error',
            position: 'bottomRight',
            icon: 'fas fa-exclamation-triangle',
            class: 'bg-danger m-1',
            autohide: true,
            delay: 3000,
            body: toast_msg,
        });
        return false;
    }
    return true;
}

function add_attribute() {
    // get form data.
    var name = $('#attribute-name').val();
    var data_type_id = $('#attribute-data-type').children(':selected').attr('id');
    var selection_id = $('#attribute-selection').children(':selected').attr('id');
    var data_type_text = $('#attribute-data-type').children(':selected').val();
    var selection_text = $('#attribute-selection').children(':selected').val();

    // validate.
    var toast_msg;
    if ((name.length <= 2) || (name.length >= 16)) toast_msg = '3~15 characters allowed.';
    if (!RegExp("^[a-zA-Z0-9_-]*$").test(name)) toast_msg = 'Invalid character.';
    if ((selection_id == 'Y') && (data_type_id != 'TEXT_ID')) toast_msg = 'Y allows NOMINAL only.';
    if (name in attribute_names) toast_msg = `${name} already exists.`;
    if ((count_y >= 1) && (selection_id == 'Y')) toast_msg = 'Only one Y is supported.';
    if (toast_msg) {
        $(document).Toasts('create', {
            title: 'Error',
            position: 'bottomRight',
            icon: 'fas fa-exclamation-triangle',
            class: 'bg-danger m-1',
            autohide: true,
            delay: 3000,
            body: toast_msg,
        });
        return;
    }

    // add to attributes.
    var html = $('#attribute-list').html();
    var node = 
`<tr id="attribute-index${index}">
    <td><input type="hidden" value="${name}/${data_type_id}/${selection_id}" name="index${index}" id="id_index${index}" />${name}</td>
    <td>${data_type_text}</td>
    <td>${selection_text}</td>
    <td class="text-right py-0 align-middle">
        <div class="btn-group btn-group-sm">
            <a href="javascript:void(0);" onclick="remove_attribute(${index})" class="btn btn-danger"><i class="fas fa-minus"></i></a>
        </div>
    </td>
</tr>`;

    html += node;
    $('#attribute-list').html(html);

    index++;
    if (selection_id == 'X') count_x++;
    if (selection_id == 'Y') count_y++;
    attribute_names[name] = true;
}

function remove_attribute(index) {
    var value = $(`#id_index${index}`).attr('value');
    var parse = value.split('/');
    if (parse.length != 3) return;
    var selection = parse[2];
    if (selection == 'X') count_x--;
    if (selection == 'Y') count_y--;

    $(`#attribute-index${index}`).remove();
    delete attribute_names[parse[0]];
}

$(function () {
    index = 0;
    count_x = 0;
    count_y = 0;

    // form validator.
    $.validator.addMethod("regex", function(value, element, regexp) {
        let re = RegExp(regexp);
        let res = re.test(value);
        return res;
    });

    $('#create_project_form').validate({
        rules: {
            project_name: {
                required: true,
                minlength: 2,
                maxlength: 16,
                regex: "^[a-zA-Z0-9_-]*$",
            },
            database_type: {
                required: true,
            }
        },
        messages: {
            project_name: {
                required: "Please enter a project name.",
                regex: "Invalid character.",
            },
            database_type: {
                required: "Please select a database type.",
            }
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
