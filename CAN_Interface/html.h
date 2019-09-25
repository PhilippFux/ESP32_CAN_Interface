R"rawText(
<!DOCTYPE html>
<html lang="en">
<head>
    <title>ESP32 Cannelloni configuration</title>
    <link rel="icon" type="image/x-icon" href="icons/setting_black.png">
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta http-equiv="X-UA-Compatible" content="ie=edge" />
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.4.0/css/bootstrap.min.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.4.0/jquery.min.js"></script>
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.4.0/js/bootstrap.min.js"></script>

    <style>
        .jumbotron {
            background-color: #f94800;
            color: #ffffff;
        }
        .config_btn {
            background-color: #f94800;
            border-color: #f94800;
            color: #ffffff;
            width: 93px;
        }
        .apply_btn {
            background-color: #f94800;
            border-color: #f94800;
            color: #ffffff;
            width:200px;
        }
        h2, .h2 {
            font-size: 1.75rem;
        }
    </style>

    <script type="text/javascript">
        var websocket;

        function WebSocketBegin() {
            if ("WebSocket" in window) {
                websocket = new WebSocket("ws://192.168.4.1:81/");
                websocket.onopen = function() {
                    websocket.send('Ping');
                };

                websocket.onmessage = function(evt) {
                    //create a JSON object
                    var jsonObject = JSON.parse(evt.data);
                };

                websocket.onclose = function() {
                    alert("Websocket connection is closed...");
                };
            }
            else {
                alert("Websocket not supported by this Browser!");
            }
        }

        var bitrate = 500, can_mode = 0, filter = 0, is_extended = 0;  // default settings
        $(document).ready(function() {
            $("#btn_apply").click( function() {
                var start_id = 0;
                var end_id = 0;

                if (filter === 1) {
                    start_id = get_start_id();
                    end_id = get_end_id();
                }

                var config = {
                    "bitrate": bitrate,
                    "can_mode": can_mode,
                    "filter": filter,
                    "is_extended": is_extended,
                    "start_id": start_id,
                    "end_id": end_id
                };
                websocket.send(JSON.stringify(config));
            });
        });

        $(document).ready(function() {
            $("#bitrate li a").click(function() {
                $("#btn_bitrate").text($(this).text());  // set selected bitrate in button for bitrate
                bitrate = $(this).data('value');
            });
        });

        $(document).ready(function() {
            $("#can_mode li a").click(function() {
                $("#btn_can_mode").text($(this).text());  // set selected can mode in button for bitrate
                can_mode = $(this).data('value');
            });
        });

        $(document).ready(function() {
            $("#filter li a").click(function() {
                $("#btn_filter").text($(this).text());  // set selected filter state in button for bitrate
                filter = $(this).data('value');
            });
        });

        $(document).ready(function() {
            $("input[type='radio']").click(function(){
                is_extended = $("input[name='frameradio']:checked").val();
            });

        });

        function get_start_id() {
            return $("#input_start_id").val();
        }

        function get_end_id() {
            return $("#input_end_id").val();
        }


        $(document).ready(function () {
            $("#filter li a").click(function () {
                if (filter === 1) {
                    $('input[id="input_start_id"]').prop('disabled', false);
                    $('input[id="input_end_id"]').prop('disabled', false);
                    $('input[name="frameradio"]').prop('disabled', false);
                }
                else {
                    $('input[id="input_start_id"]').prop('disabled', true);
                    $('input[id="input_end_id"]').prop('disabled', true);
                    $('input[name="frameradio"]').prop('disabled', true);
                }
            });
        });
    </script>
</head>

<body onLoad="javascript:WebSocketBegin()">
<div class="jumbotron">
    <div class="row">
        <div class="col-sm-8"><h1>ESP32 Cannelloni configuration</h1>
            <p>CAN-Bus settings for the ESP32 Cannelloni CAN-Interface</p></div>
        <div class="col-sm-4"><img src="icons/setting_white.png" alt="setting" style="width:27%" class="pull-right"></div>
    </div>
</div>
<div>
    <div class="table-striped">
        <table class="table table-striped">
            <thead>
            <tr>
            </tr>
            </thead>
            <tbody>
            <tr>
                <td><h2>Nominal Bitrate</h2></td>
                <td>
                    <div class="dropdown">
                        <button class="btn dropdown-toggle config_btn" type="button" data-toggle="dropdown" id="btn_bitrate">
                            <span class="caret"></span></button>
                        <u1 class="dropdown-menu" id="bitrate">
                            <li><a href="#" data-value="250">250 kBit/s</a></li>
                            <li><a href="#" data-value="500">500 kBit/s</a></li>
                            <li><a href="#" data-value="1000">1 MBit/s</a></li>
                        </u1>
                    </div>
                </td>
                <td></td>
                <td></td>
            </tr>
            <tr>
                <td><h2>CAN Mode</h2></td>
                <td>
                    <div class="dropdown">
                        <button class="btn dropdown-toggle config_btn" type="button" data-toggle="dropdown" id="btn_can_mode">
                            <span class="caret"></span></button>
                        <u1 class="dropdown-menu" id="can_mode">
                            <li><a href="#" data-value="0">Normal</a></li>
                            <li><a href="#" data-value="1">No Ack</a></li>
                            <li><a href="#" data-value="2">Listen Only</a></li>
                        </u1>
                    </div>
                </td>
                <td></td>
                <td></td>
            </tr>
            <tr>
                <td><h2>Filter Settings</h2></td>
                <td>
                    <div class="dropdown">
                        <button class="btn dropdown-toggle config_btn" type="button" data-toggle="dropdown" id="btn_filter">
                            <span class="caret"></span></button>
                        <u1 class="dropdown-menu" id="filter">
                            <li><a href="#" data-value="1">on</a></li>
                            <li><a href="#" data-value="0">off</a></li>
                        </u1>
                    </div>
                </td>
                <td>
                    <div class="radio">
                        <label><input type="radio" name="frameradio" value="0" disabled>Standard</label>
                    </div>
                    <div class="radio">
                        <label><input type="radio" name="frameradio" value="1" disabled>Extended</label>
                    </div>
                </td>
                <td>
                    <div class="form-group row">
                        <label class="col-sm-1 control-label" for="input_start_id">from:</label>
                        <div class="col-xs-2">
                            <input type="text" class="form-control" id="input_start_id" placeholder="Hex" disabled>
                        </div>
                    </div>
                    <div class="from-group row">
                        <label class="col-sm-1 control-label" for="input_end_id">to:</label>
                        <div class="col-xs-2">
                            <input type="text" class="form-control" id="input_end_id" placeholder="Hex" disabled>
                        </div>
                    </div>
                </td>
            </tr>
            </tbody>
        </table>
    </div>
</div>
<div class="button">
    <button
            class="btn center-block apply_btn"
            type="button"
            id="btn_apply"><img src="icons/save_round_white.png" alt="setting" style="width:11%"> Apply Configuration
    </button>
</div>
</body>
</html>
)rawText"
