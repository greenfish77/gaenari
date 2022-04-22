
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
`<tr>
    <td><input type="hidden" value="${name}/${data_type_id}/${selection_id}" name="index${index}" />${name}</td>
    <td>${data_type_text}</td>
    <td>${selection_text}</td>
    <td class="text-right py-0 align-middle">
        <div class="btn-group btn-group-sm">
            <a href="#" class="btn btn-danger"><i class="fas fa-trash"></i></a>
        </div>
    </td>
</tr>`;

    html += node;
    $('#attribute-list').html(html);

    index++;
}

