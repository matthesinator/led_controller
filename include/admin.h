static const char ADMIN[] PROGMEM = R"=====(
<!DOCTYPE html>

<html>

<head>
    <title>LED Admin Interface</title>

    <style type="text/css">
        .buttons {
            font-family: Arial;
            background: #d3d3d3;
            color: #000000;
            text-shadow: 1px 1px 2px #ffffff;
            border: solid #000000 1px;
            font-weight: 300;
            margin-top: 5px;
            font-size: 1.5em;
            width: 49%;
            height: 50px;
        }

        .buttons.lock {

        }

        .buttons.unlock {
        	float: right;
        }
        
        .answer {
            font-family: Arial;
            font-size: 1.5em;
            text-align: center;
        }
        
        .password {
            font-family: Arial;
            font-size: 1.5em;
            margin-top: 5px;
            box-sizing: border-box;
            width: 100%;
            height: 45px;
            border: solid #000000 1px;
        }
        
        .error {
            border: 2px solid red;
        }
        
        .error::placeholder {
            color: red;
        }

        .bigDiv {
        	width: 25%;
        }
    </style>
</head>

<body>
	<div class="bigDiv">
    	<input type="button" class="buttons lock" value="Lock" onclick="toggle('lock')">
    	<input type="button" class="buttons unlock" value="Unlock" onclick="toggle('unlock')">
    	<br>
    	<input type="password" placeholder="Password" id="password" class="password">
    	<div id="div" class="answer" background="red"></div>
	</div>

    <script type="text/javascript">
        function toggle(what) {
            var password = document.getElementById("password").value;
            if (!password == "") {
                document.getElementById("password").className = document.getElementById("password").className.replace(" error", "");
                var data = "password=";
                data += password;
                data += "&request=";
                data += what;
                sendRequest(data);
            } else {
                document.getElementById("password").placeholder = "Enter Password";
                document.getElementById("password").className = document.getElementById("password").className + " error"
            }
        }

        function sendRequest(data) {
            var url = "/settings?";
            url += data;

            var http = new XMLHttpRequest();
            http.open("GET", url);
            http.addEventListener('load', function(event) {
                if (http.status >= 200 && http.status < 300) {
                    correctPassword(http.responseText);
                } else {
                    wrongPassword();
                }
            });
            http.send();
        }

        function correctPassword(answer) {
            document.getElementById("div").innerHTML = answer;
        }

        function wrongPassword() {
            document.getElementById("password").value = "";
            document.getElementById("password").placeholder = "Wrong Password";
            document.getElementById("password").className = document.getElementById("password").className + " error";
        }
    </script>
</body>
    )=====";