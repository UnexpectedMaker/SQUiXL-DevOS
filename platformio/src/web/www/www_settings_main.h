#pragma once

const char index_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE html>
	<html data-bs-theme="%THEME%">
		<head>
			<title>SQUiXL | Web Portal</title>
			%META%
		</head>
		<body>
			<div class="content" style="margin-top:12px;">
				<h2 class="center">SQUiXL | General Settings</h2>
	
					<center>
						<img src="https://d2j6dbq0eux0bg.cloudfront.net/images/wysiwyg/product/90477757/743870537/17454876405541911181609/SQUiXL_Logo_Blue_Website_Transparent_png.png" style="width:10%; height: 10%; margin:10px;">
					
                        <div class="content" style="margin:10px;">
                            <div class="row">
                                <div class='col-2'>&nbsp;</div>
                                <div class='col-8'>

                                    <div class="row menu">
                                        <div class='col-3 center'><span>GENERAL</span></div>
                                        <div class='col-3 center'><a href='/wifi'>WiFi&nbsp;&&nbsp;WEB</a></div>
                                        <div class='col-3 center'><a href='/widgets'>WIDGETS</a></div>
                                        <div class='col-3 center'><a href='/wallpaper'>WALLPAPER</a></div>
                                    </div>

                                </div>
                                <div class='col-2'>&nbsp;</div>
                            </div>
                        </div>
                    </center>

				<div class="content">
					%SETTING_OPTIONS_MAIN%
				</div>

			%FOOTER%
	
			<script>
				document.addEventListener('htmx:afterSwap', function(evt) {
					// Ensure the target container is one of the form containers
					if (evt.detail.target.id.startsWith("settings_group_")) {
						var flashSpan = evt.detail.target.querySelector('.flash-span');
						if (flashSpan) {
							flashSpan.style.display = 'inline';
							flashSpan.classList.add('flash_post');
							setTimeout(function() {
								flashSpan.classList.remove('flash_post');
								flashSpan.style.display = 'none';
							}, 1000); // Adjust the duration as needed
						}
					}
				});

			</script>
	
	)rawliteral";

const char index_wifi_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE html>
	<html data-bs-theme="%THEME%">
		<head>
			<title>SQUiXL | Web Portal</title>
			%META%
		</head>
		<body>
			<div class="content" style="margin-top:12px;">
				<h2 class="center">SQUiXL | WiFi & Web Settings</h2>
	
					<center>
						<img src="https://d2j6dbq0eux0bg.cloudfront.net/images/wysiwyg/product/90477757/743870537/17454876405541911181609/SQUiXL_Logo_Blue_Website_Transparent_png.png" style="width:10%; height: 10%; margin:10px;">
					
                        <div class="content" style="margin:10px;">
                            <div class="row">
                                <div class='col-2'>&nbsp;</div>
                                <div class='col-8'>

                                    <div class="row menu">
                                        <div class='col-3 center'><a href='/'>GENERAL</a></div>
                                        <div class='col-3 center'><span>WiFi&nbsp;&&nbsp;WEB</span></div>
                                        <div class='col-3 center'><a href='/widgets'>WIDGETS</a></div>
                                        <div class='col-3 center'><a href='/wallpaper'>WALLPAPER</a></div>
                                    </div>

                                </div>
                                <div class='col-2'>&nbsp;</div>
                            </div>
                        </div>
                    </center>

				<div class="content">
					%SETTING_OPTIONS_WEB%
				</div>

			%FOOTER%
	
			<script>
				document.addEventListener('htmx:afterSwap', function(evt) {
					// Ensure the target container is one of the form containers
					if (evt.detail.target.id.startsWith("settings_group_")) {
						var flashSpan = evt.detail.target.querySelector('.flash-span');
						if (flashSpan) {
							flashSpan.style.display = 'inline';
							flashSpan.classList.add('flash_post');
							setTimeout(function() {
								flashSpan.classList.remove('flash_post');
								flashSpan.style.display = 'none';
							}, 1000); // Adjust the duration as needed
						}
					}
				});

			</script>
	
	)rawliteral";

const char index_widgets_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE html>
	<html data-bs-theme="%THEME%">
		<head>
			<title>SQUiXL | Web Portal</title>
			%META%
		</head>
		<body>
			<div class="content" style="margin-top:12px;">
				<h2 class="center">SQUiXL | Widget Settings</h2>
	
					<center>
						<img src="https://d2j6dbq0eux0bg.cloudfront.net/images/wysiwyg/product/90477757/743870537/17454876405541911181609/SQUiXL_Logo_Blue_Website_Transparent_png.png" style="width:10%; height: 10%; margin:10px;">
					
                        <div class="content" style="margin:10px;">
                            <div class="row">
                                <div class='col-2'>&nbsp;</div>
                                <div class='col-8'>

                                    <div class="row menu">
                                        <div class='col-3 center'><a href='/'>GENERAL</a></div>
                                        <div class='col-3 center'><a href='/wifi'>WiFi&nbsp;&&nbsp;WEB</a></div>
                                        <div class='col-3 center'><span>WIDGETS</span></div>
                                        <div class='col-3 center'><a href='/wallpaper'>WALLPAPER</a></div>
                                    </div>

                                </div>
                                <div class='col-2'>&nbsp;</div>
                            </div>
                        </div>
                    </center>

				<div class="content">
					%SETTING_OPTIONS_WIDGETS%
				</div>
				
			%FOOTER%
	
			<script>
				document.addEventListener('htmx:afterSwap', function(evt) {
					// Ensure the target container is one of the form containers
					if (evt.detail.target.id.startsWith("settings_group_")) {
						var flashSpan = evt.detail.target.querySelector('.flash-span');
						if (flashSpan) {
							flashSpan.style.display = 'inline';
							flashSpan.classList.add('flash_post');
							setTimeout(function() {
								flashSpan.classList.remove('flash_post');
								flashSpan.style.display = 'none';
							}, 1000); // Adjust the duration as needed
						}
					}
				});

			</script>
	
	)rawliteral";

const char screenie_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE html>
	<html data-bs-theme="%THEME%">
		<head>
			<title>SQUiXL | Web Portal</title>
			%META%
		</head>
		<body>
			<div class="content" style="margin-top:12px;">
				<h2 class="center">SQUiXL | Screenie</h2>
	
					<center>
						<img src="https://d2j6dbq0eux0bg.cloudfront.net/images/wysiwyg/product/90477757/743870537/17454876405541911181609/SQUiXL_Logo_Blue_Website_Transparent_png.png" style="width:10%; height: 10%; margin:10px;">
					
                        <div class="content" style="margin:10px;">
                            <div class="row">
                                <div class='col-2'>&nbsp;</div>
                                <div class='col-8'>

                                    <div class="row menu">

                                        <div class='col-3 center'><a href='/'>GENERAL</a></div>
                                        <div class='col-3 center'><a href='/wifi'>WiFi&nbsp;&&nbsp;WEB</a></div>
                                        <div class='col-3 center'><a href='/widgets'>WIDGETS</a></div>
                                        <div class='col-3 center'><a href='/wallpaper'>WALLPAPER</a></div>
                                    </div>

                                </div>
                                <div class='col-2'>&nbsp;</div>
                            </div>
                        </div>
                    </center>

				<div class="content" style="text-align: center;">
					<img src="/screenshot">
                </div>
                <div class="content" style="text-align: center; margin-top:20px;">
                    <button class="btn btn-sm btn-primary m-1" hx-get="/take_screenshot">Take New Screenshot</button>
				</div>

                <div class="content">
					%SETTING_SCREENIE%
				</div>
                 <div class="content" style="text-align: center;">
					<p style="font-size:13px;"><strong>WARNING:</strong> The more processing you change from the default "center" of each setting, the longer screenshots will take to process, locking you out of the SQUiXL UI and Web settings until complete.</p>
                    <p style="font-size:13px;">SQUiXL will make an audio ping sound once the screenshot has been generated, and then the page will automatically update.</p>
				</div>
	
	
			%FOOTER%
	
			<script>
				document.addEventListener('htmx:afterSwap', function(evt) {
					// Ensure the target container is one of the form containers
					if (evt.detail.target.id.startsWith("settings_group_")) {
						var flashSpan = evt.detail.target.querySelector('.flash-span');
						if (flashSpan) {
							flashSpan.style.display = 'inline';
							flashSpan.classList.add('flash_post');
							setTimeout(function() {
								flashSpan.classList.remove('flash_post');
								flashSpan.style.display = 'none';
							}, 1000); // Adjust the duration as needed
						}
					}
				});

                if (!!window.EventSource)
                {
                    var source = new EventSource('/event');
                    source.onopen = function(e) {
                        console.log("Events Connected");
                    };
                    source.addEventListener('refresh', function(e) {
                        console.log("refresh");
                        window.location.reload();
                    }, false);
                    source.addEventListener('message', function(e) {
                        console.log("message", e.data);
                    }, false);
                    source.addEventListener('heartbeat', function(e) {
                        console.log("heartbeat", e.data);
                    }, false);
                }
			</script>
	
	)rawliteral";

const char wallpaper_html[] PROGMEM = R"rawliteral(
	<!DOCTYPE html>
	<html data-bs-theme="%THEME%">
		<head>
			<title>SQUiXL | Web Portal</title>
			%META%
		</head>
		<body>
			<div class="content" style="margin-top:12px;">
				<h2 class="center">SQUiXL | Wallpaper Uploader</h2>
	
					<center>
						<img src="https://d2j6dbq0eux0bg.cloudfront.net/images/wysiwyg/product/90477757/743870537/17454876405541911181609/SQUiXL_Logo_Blue_Website_Transparent_png.png" style="width:10%; height: 10%; margin:10px;">
					
                        <div class="content" style="margin:10px;">
                            <div class="row">
                                <div class='col-2'>&nbsp;</div>
                                <div class='col-8'>

                                    <div class="row menu">

                                        <div class='col-3 center'><a href='/'>GENERAL</a></div>
                                        <div class='col-3 center'><a href='/wifi'>WiFi&nbsp;&&nbsp;WEB</a></div>
                                        <div class='col-3 center'><a href='/widgets'>WIDGETS</a></div>
                                        <div class='col-3 center'><span>WALLPAPER</span></div>
                                    </div>

                                </div>
                                <div class='col-2'>&nbsp;</div>
                            </div>
                        </div>
                    </center>

				<div class="content" style="text-align: center;">
                    <p>Current User Wallpaper</p>
					<img src="/get_wallpaper">
                </div>
                <div class="content" style="text-align: center; margin-top:20px;">
                
                <form id='uploadForm' hx-encoding='multipart/form-data' hx-swap='none' hx-post='/upload_wallpaper'
          _='on htmx:xhr:progress(loaded, total) set #progress.value to (loaded/total)*100
             on htmx:afterRequest if not window.blockReload then window.location.reload()'>

                    <div class="mb-3">
                        <label for="formFile" class="form-label">Select a JPG file that is sized 480x480 and is under 200KB in size</label>
                        <input class="form-control" type="file" id="formFile" name='user_wallpaper.jpg' accept=".jpg">
                         <button id="uploadBtn" class="btn btn-sm btn-primary m-1" style="display:block;" disabled>Upload Wallpaper</button>
                    </div>
                   
                    <br>
                    <progress id="progress" value="0" max="100" style="display:none;width:100%%;"></progress>
                </form>

                <div id="file-error" style="color:red;margin-top:5px;font-size:18px;"></div>
                <div id="image-dimensions" style="margin-top:5px;font-size:18px;"></div>
                    
				</div>

                <!--
                <div class="content">

                FILELIST

				</div>
                -->

                <div class="content" style="margin-top:12px; text-align:center; color:#bbb;">
                    <h3>User Storage: <strong>%LFS_FREE%</strong> | Used Storage: <strong>%LFS_USED%</strong> | Total Storage: <strong>%LFS_TOTAL%</strong></h3>
                </div>
	
			%FOOTER_WALLPAPER%
	
	)rawliteral";

const char index_html_old[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html data-bs-theme="%THEME%">
	<head>
		<title>SQUiXL | Web Settings</title>
		%META%
	</head>
	<body>
		<div class="content" style="margin-top:12px;">
			<h2 class="center">SQUiXL | Web Portal | Main</h2>

			<div class="content" style="margin:30px;">
				<div class="row">
					<div class='col-1'>&nbsp;</div>
					<div class='col-2 center'><button class='btn btn-sm btn-primary' style='width:130px;'>MAIN</button></div>
					<div class='col-2 center'><a href='/web_settings_web.html'><button class='btn btn-sm btn-outline-primary' style='width:130px;'>WIFI&nbsp;&&nbsp;WEB</button></a></div>
					<div class='col-2 center'><a href='/web_settings_widgets.html'><button class='btn btn-sm btn-outline-primary' style='width:130px;'>WIDGETS</button></a></div>
					<div class='col-2 center'><a href='/web_settings_apps.html'><button class='btn btn-sm btn-outline-primary' style='width:130px;'>APPS</button></a></div>
					<div class='col-2 center'><a href='/web_settings_themes.html'><button class='btn btn-sm btn-outline-primary' style='width:130px;'>THEMES</button></a></div>
					<div class='col-1'>&nbsp;</div>
				</div>
			</div>

			<div class="content">
				
				%SETTING_OPTIONS_WATCH%

			</div>

		%FOOTER%

		<script>
			document.addEventListener('htmx:afterSwap', function(evt) {
				// Ensure the target container is one of the form containers
				if (evt.detail.target.id.startsWith("settings_group_")) {
					var flashSpan = evt.detail.target.querySelector('.flash-span');
					if (flashSpan) {
						flashSpan.style.display = 'inline';
						flashSpan.classList.add('flash_post');
						setTimeout(function() {
							flashSpan.classList.remove('flash_post');
							flashSpan.style.display = 'none';
						}, 1000); // Adjust the duration as needed
					}
				}
			});
		</script>

)rawliteral";

const char settings_group_0_description[] PROGMEM = R"rawliteral(

)rawliteral";
