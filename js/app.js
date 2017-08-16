$(document).ready(function () {
	$.ajax({
		url: "../data.php",
		method: "GET",
		success: function (data) {
			// console.log(data);
			var ID = [];
			var co2 = [];
			var date = [];

			for (var i in data) {
				ID.push(data[i].id);
				co2.push(data[i].co2);
				date.push(data[i].date);
			}

			var chartdata = {
				labels: date,
				datasets: [
					{
						label: 'CO2 Level',
						fill: false,
						lineTension: 0.1,
						backgroundColor: "rgba(59, 89, 152, 0.75)",
						borderColor: "rgba(59, 89, 152, 1)",
						pointHoverBackgroundColor: "rgba(59, 89, 152, 1)",
						pointHoverBorderColor: "rgba(59, 89, 152, 1)",
						data: co2, ID
					}
				]
			};

			var ctx = $("#mycanvas");

			var lineGraph = new Chart(ctx, {
				type: 'line',
				data: chartdata
			});
		},
		error: function (data) {
			console.log(data);
			console.log("There was an unexpected error, please try again");
		}
	});
});