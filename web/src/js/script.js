var webSocket = new WebSocket("ws://" + location.hostname + ":81");
webSocket.onmessage = function (e) {
    var message = e.data;
    $('#log-container').find('>pre').append(message);
};


var loadConfig = function () {
    $.ajax(
        '/config',
        {
            method: 'GET',
            contentType: 'application/json'
        }
    ).done(function (data) {
               $.each(data, function (key, value) {
                   $(".config-item[data-field=" + key + "]").val(value);
               });
           }
    );
};


var sendCommand = _.throttle(
    function (params) {
        $.ajax(
            '/system',
            {
                method: 'POST',
                data: JSON.stringify(params),
                contentType: 'application/json'
            }
        );
    },
    1000
);


var sendConfig = function (config) {
    $.ajax(
        '/config',
        {
            method: 'PUT',
            data: JSON.stringify(config),
            contentType: 'application/json'
        }
    );
};


$(function () {
    $('.system-btn').click(function () {
        sendCommand({command: $(this).data('command')});
    });

    $('#web-log-level').change(function () {
        sendConfig({webLogLevel: $('#web-log-level').val()});
    });

    loadConfig();
});
