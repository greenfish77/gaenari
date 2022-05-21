
// ready.
$(function () {
	/*
	$.ajax({
		url: '/api/v1/report',
		type: 'GET',
	}).done((data, textStatus, jqXHR) => {
		// {error: true, errormsg: 'empty instance'}
		// {error: true, errormsg: ...}
		// {error: false, ...}
		if (data.error) {
			// alert(data.errormsg);
		}
	}).fail((jqXHR, textStatus, errorThrown) => {
	}).always((param1, param2, param3) => {
	});
	*/

	$.ajax({
		url: '/api/v1/project/name',
		type: 'GET',
	}).done((data, textStatus, jqXHR) => {
		$('#project_name').html(data.project_name);
	}).fail((jqXHR, textStatus, errorThrown) => {
	}).always((param1, param2, param3) => {
	});

	$.ajax({
		url: '/api/v1/global',
		type: 'GET',
	}).done((data, textStatus, jqXHR) => {
		$('#instance_count').html(data.instance_count);
		$('#updated_instance_count').html(data.updated_instance_count);
		$('#instance_accuracy').html(data.instance_accuracy);
		$('#instance_accuracy_progress').width(`${data.instance_accuracy*100}%`);
	}).fail((jqXHR, textStatus, errorThrown) => {
	}).always((param1, param2, param3) => {
	});

	$('.connectedSortable').sortable({
		placeholder: 'sort-highlight',
		connectWith: '.connectedSortable',
		handle: '.card-header, .nav-tabs',
		forcePlaceholderSize: true,
		zIndex: 999999
	});
	$('.connectedSortable .card-header').css('cursor', 'move');

	// jQuery UI sortable for the todo list
	$('.todo-list').sortable({
		placeholder: 'sort-highlight',
		handle: '.handle',
		forcePlaceholderSize: true,
		zIndex: 999999
	});
});
