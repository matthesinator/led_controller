static const char GUI[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8"/>
<title>LED Control</title>
<script src=https://cdn.jsdelivr.net/npm/@jaames/iro/dist/iro.min.js></script>
<script src=colorpicker.js></script>
<link rel=stylesheet href=colorpicker.css/>
</head>
<body>
<div class=content>
<div id=color-picker-container></div>
<hr>
<div class=buttoncontainer>
<div class=buttonrow>
<input type=button class=button id=toggle value="OFF/ON"/>
<input type=button class="button greyed" id=tv value="Fernseher"/>
<input type=button class="button greyed" id=bett value="Bett"/>
</div>
<div class=buttonrow>
<input type=button class=button id=blink value="BLINK"/>
<input type=button class=button id=fade value="FADE"/>
<input type=button class=button id=jump value="JUMP"/>
</div>
</div>
<hr>
<div class=slidecontainer>
<input type=range max=9 min=0 step=1 class=range id="speed"/>
</div>
<p class=answer>nix</p>
</div>
</body>
</html>
)=====";