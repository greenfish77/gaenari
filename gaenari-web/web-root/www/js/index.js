
$(function () {
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

	$.ajax({
		url: '/api/v1/project/name',
		type: 'GET',
	}).done((data, textStatus, jqXHR) => {
		$('#project_name').html(data.project_name);
	}).fail((jqXHR, textStatus, errorThrown) => {
	}).always((param1, param2, param3) => {
	});
});
