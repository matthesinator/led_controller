const char GUI[] PROGMEM = R"=====(
<html>
<!--if somethings not working, comment out the http request at the bottom-->
<head>
    <!-- ... -->
    <title>LED Web Interface</title>
    <style type="text/css">
        html, body {
            overflow: hidden;
        }

        body {
            position: relative;
        }
        
        .btn_confirm {
            font-family: Arial;
            background: #ffffff;
            color: #333333;
            text-shadow: 1px 1px 2px #ffffff;
            border: solid #000000 1px;
            font-weight: 300;
            font-size: 1.5em;
        }
        
        .btn_turnoff {
            font-family: Arial;
            background: #700000;
            color: #ffffff;
            text-shadow: 1px 1px 2px #555555;
            border: solid #000000 1px;
            font-weight: 300;
            font-size: 1.5em;
        }
        
        .btn_mode {
            font-family: Arial;
            background: #d3d3d3;
            color: #000000;
            text-shadow: 1px 1px 2px #ffffff;
            border: solid #000000 1px;
            font-weight: 300;
            margin-top: 5px;
            font-size: 1.5em;
        }

        .btn_preset {
        	font-family: Arial;
        	background: #d3d3d3;
        	color: #000000;
        	text-shadow: 1px 1px 2px #ffffff;
            border: solid #000000 1px;
            font-weight: 300;
            font-size: 1.5em;
        }

        .btn_save_preset {
            font-family: Arial;
            background: #d3d3d3;
            color: #000000;
            text-shadow: 1px 1px 2px #ffffff;
            border: solid #000000 1px;
            font-weight: 300;
            font-size: 1em;
            margin-top: 1px;
        }
        
        .slidecontainer {
            width: 100%;
        }
        
        .colorslidecontainer {
            width: 100%;
        }
        
        .slider {
            margin-top: 0;
            -webkit-appearance: none;
            height: 24px;
            background: #d3d3d3;
            border: 1px solid #000000;
            border-radius: 50px;
        }
        
        .slider#redSlider {
            background: linear-gradient(to right, black, red);
        }
        
        .slider#greenSlider {
            background: linear-gradient(to right, black, green);
        }
        
        .slider#blueSlider {
            background: linear-gradient(to right, black, blue);
        }
        
        #sliderPart {
            display: inline;
        }
        
        #redVal {
            display: inline-block;
            align-content: right;
        }
        
        #greenVal {
            display: inline-block;
            align-content: right;
        }
        
        #blueVal {
            display: inline-block;
            align-content: right;
        }
        
        .slider::-webkit-slider-thumb {
            height: 10px;
        }
        
        .slider::-moz-range-thumb {
            height: 18px;
            width: 18px;
            background: #d3d3d3;
            border: 2px solid #000000;
            border-radius: 50px;
        }
        
        .text {
            font-family: Arial;
            font-size: 1.5em;
            width: 50%;
        }
        
        input {
            width: 16%;
            height: 5%;
            border: 1px solid black;
            background: #d3d3d3;
            text-align: center
        }

        input[type="checkbox"] {
        	height: 1em;
        	width: 1em;
        	float: right;
        }
    </style>
</head>

<body onload="fitElements()">
    <div class="wheel" id="color-picker-container"></div>
    <br>
    <input type="button" class="btn_confirm" id="btn_confirm" onclick="setColor()" value="CONFIRM">
    <input type="button" class="btn_turnoff" id="btn_turnoff" onclick="turnOff()" value="OFF">
    <br>
    <input type="button" class="btn_mode" onclick="setMode('blink')" value="BLINK">
    <input type="button" class="btn_mode" onclick="setMode('fade')" value="FADE">
    <input type="button" class="btn_mode" onclick="setMode('jump')" value="JUMP">
    <br>
    <br>
    <div class="text" id="boxWidth">Speed:<input type="checkbox" id="leftbox" checked><input type="checkbox" id="rightbox" checked></div>
    <div class="slidecontainer">
        <input type="range" min="1" max="10" value="5" class="slider" id="speedSlider">
    </div>
    <br>
    <div class="colorslidecontainer">
        <div id="sliderPart">
            <input type="range" min="1" max="18" value="5" class="slider" id="redSlider">
        </div>
        <div class="text" id="redVal"></div>
    </div>
    <div class="colorslidecontainer">
        <div id="sliderPart">
            <input type="range" min="1" max="18" value="5" class="slider" id="greenSlider">
        </div>
        <div class="text" id="greenVal"></div>
    </div>
    <div class="colorslidecontainer">
        <div id="sliderPart">
            <input type="range" min="1" max="18" value="5" class="slider" id="blueSlider">
        </div>
        <div class="text" id="blueVal"></div>
    </div>
    <br>
    <input type="button" class="btn_preset" onclick="setPreset(0)">
    <input type="button" class="btn_preset" onclick="setPreset(1)">
    <input type="button" class="btn_preset" onclick="setPreset(2)">
    <input type="button" class="btn_preset" onclick="setPreset(3)">
    <br>
    <input type="button" class="btn_save_preset" onclick="savePreset(0)" value="SAVE">
    <input type="button" class="btn_save_preset" onclick="savePreset(1)" value="SAVE">
    <input type="button" class="btn_save_preset" onclick="savePreset(2)" value="SAVE">
    <input type="button" class="btn_save_preset" onclick="savePreset(3)" value="SAVE">

    <p id="link"></p>
    <p id="response"></p>
    <script src="iro.min.js"></script>
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>
    <!--Should work offline. To reduce needed storage, use https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js-->
    <script type="text/javascript">
        var size = getSize();

        var demoColorPicker = new iro.ColorPicker("#color-picker-container", {
            width: size,
            borderColor: "#000000",
            borderWidth: 1,
            padding: 2,
            wheelLightness: false,
            sliderMargin: 5
        });

        var red = 255;
        var green = 255;
        var blue = 255;

        var counter = 0;
        var url = "http://192.168.2.113/";
        var url2 = "http://192.168.2.111/"

        demoColorPicker.on("color:change", function(color) {
            document.getElementById("btn_confirm").style.backgroundColor = demoColorPicker.color.hexString;
            
            if (counter == 5) {
                red = color.rgb.r;
                green = color.rgb.g;
                blue = color.rgb.b;

                setColor();
                syncSlider();
                counter = 0;
            } else {
                counter++;
            }
        });

        demoColorPicker.on("input:end", function(color) {
            red = color.rgb.r;
            green = color.rgb.g;
            blue = color.rgb.b;
            setColor();
            syncSlider();
        });

        var speedSlider = document.getElementById("speedSlider");
        speedSlider.oninput = function() {
            var position = speedSlider.value - 1;
            sendRequest("request?mode=speed&newSpeed=" + position);
        }

        var redSlider = document.getElementById("redSlider");
        redSlider.oninput = function() {
            var position = (redSlider.value - 1) * 15;
            document.getElementById("redVal").innerHTML = position;
            updateColor(position, green, blue);
            setColor();
            syncWheel();
        }

        var greenSlider = document.getElementById("greenSlider");
        greenSlider.oninput = function() {
            var position = (greenSlider.value - 1) * 15;
            document.getElementById("greenVal").innerHTML = position;
            updateColor(red, position, blue);
            setColor();
            syncWheel();
        }

        var blueSlider = document.getElementById("blueSlider");
        blueSlider.oninput = function() {
            var position = (blueSlider.value - 1) * 15;
            document.getElementById("blueVal").innerHTML = position;
            updateColor(red, green, position);
            setColor();
            syncWheel();
        }

        function setMode(mode) {
            var data = "request?mode=";
            data += mode;
            sendRequest(data);
        }

        function setPreset(index) {
        	var data = "request?mode=preset&index=";
        	data += index;
        	sendRequest(data);
        }

        function savePreset(index) {
            var data = "request?mode=preset&save=true&index=";
            data += index;
            sendRequest(data);
            syncPresets();
        }

        function updateColor(newRed, newGreen, newBlue) {
            red = newRed;
            green = newGreen;
            blue = newBlue;
        }

        function turnOff() {
            var offData = "request?mode=color&red=0&green=0&blue=0";
            sendRequest(offData);
        }

        function setColor() {
            var colorData = "request?mode=color&red=" + red + "&green=" + green + "&blue=" + blue;
            sendRequest(colorData);
        }

        function sendRequest(data) {
        	var leftBox = document.getElementById("leftbox");
        	if (leftBox.checked == true) {
            	var http = new XMLHttpRequest();
            	var total = url + data;
            	http.open("POST", total);
            	http.send();
            }

            var rightBox = document.getElementById("rightbox");
            if (rightBox.checked == true) {
            	var http2 = new XMLHttpRequest();
            	var total = url2 + data;
            	http2.open("POST", total);
            	http2.send();
            	document.getElementById("link").innerHTML = total;
        	}
        }

        function getSize() {
            if (window.innerWidth > window.innerHeight) {
                return window.innerHeight * 0.5;
            } else {
                return window.innerWidth * 0.5;
            }
        }

        function syncWheel() {
            demoColorPicker.color.rgb = {
                r: red,
                g: green,
                b: blue
            };

            var btns = document.getElementsByClassName("btn_save_preset");
            for (i = 0; i < btns.length; i++) {
                btns[i].style.backgroundColor = demoColorPicker.color.hexString;
            }
        }

        function syncSlider() {
            document.getElementById("redSlider").value = (red + 15) / 15;
            document.getElementById("redVal").innerHTML = red;
            document.getElementById("greenSlider").value = (green + 15) / 15;
            document.getElementById("greenVal").innerHTML = green;
            document.getElementById("blueSlider").value = (blue + 15) / 15;
            document.getElementById("blueVal").innerHTML = blue;

            var btns = document.getElementsByClassName("btn_save_preset");
            for (i = 0; i < btns.length; i++) {
                btns[i].style.backgroundColor = demoColorPicker.color.hexString;
            }
        }

        function syncPresets() {
            var http2 = new XMLHttpRequest();
            http2.open("GET", "http://192.168.2.111/get?arg=presets", false);
            http2.send();

            document.getElementById("response").innerHTML = http2.responseText;
            var args = http2.responseText.split(";");
            var counter = 0;

            btns = document.getElementsByClassName("btn_preset");
            for (i = 0; i < btns.length; i++) {
                var red = args[counter];
                counter++;
                var green = args[counter];
                counter++;
                var blue = args[counter];
                counter++;

                var color = "rgb(";
                color += red + "," + green + "," + blue + ")";

                btns[i].style.backgroundColor = color;
            }
        }

        function fitElements() {
            document.getElementById("color-picker-container").style.width = size;
            document.getElementById("btn_confirm").style.width = size / 2 - 2;
            document.getElementById("btn_confirm").style.height = size / 5;
            document.getElementById("btn_turnoff").style.width = size / 2 - 2;
            document.getElementById("btn_turnoff").style.height = size / 5;

            var btns = document.getElementsByClassName("btn_mode");
            for (i = 0; i < btns.length; i++) {
                btns[i].style.width = size / 3 - 3;
                btns[i].style.height = size / 5;
            }
            document.getElementById("speedSlider").style.width = size;
            document.getElementById("boxWidth").style.width = size;

            var sliders = document.getElementsByClassName("colorslidecontainer");
            for (i = 0; i < btns.length; i++) {
                sliders[i].style.width = size * 5.6; //idk why the size is suddenly weird
            }

            btns = document.getElementsByClassName("btn_preset");
            for (i = 0; i < btns.length; i++) {
            	btns[i].style.width = size / 4 - 3;
                btns[i].style.height = size / 6;
            }

            btns = document.getElementsByClassName("btn_save_preset");
            for (i = 0; i < btns.length; i++) {
                btns[i].style.width = size / 4 - 3;
                btns[i].style.height = size / 15;
            }

            document.getElementById("redSlider").value = red;
            document.getElementById("greenSlider").value = green;
            document.getElementById("blueSlider").value = blue;

            document.getElementById("redVal").innerHTML = red;
            document.getElementById("greenVal").innerHTML = green;
            document.getElementById("blueVal").innerHTML = blue;

            var http = new XMLHttpRequest();

            //If sth not working, comment out this block
            http.open("GET", "http://192.168.2.111/get?arg=current", false);
        	http.send();


        	document.getElementById("response").innerHTML = http.responseText;
        	var args = http.responseText.split(";");
        	red = parseInt(args[0], 10);
        	green = parseInt(args[1], 10);
        	blue = parseInt(args[2], 10);
        	var speed = parseInt(args[3], 10);

            document.getElementById("speedSlider").value = speed
            syncWheel();
            syncSlider();
            syncPresets();
        }
    </script>
</body>

</html>
)=====";