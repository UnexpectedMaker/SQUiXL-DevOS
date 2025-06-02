#pragma once

const char footer[] PROGMEM = R"rawliteral(
		<div class="footer" %FOOTER_STYLE%>
            %UPDATE_NOTICE%
			<span>SQUiXL - 2025 Unexpected Maker - %FW_VER%</span><br>
			<span>Firmware Source available on <a href="https://github.com/squixl" target="_blank">on GitHUB</a></span>
			<!--<span>&nbsp;&nbsp;<a href="/debug_logs.html">debug logs</a></span>-->
		</div>
		<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz" crossorigin="anonymous"></script>
        <script src="https://unpkg.com/hyperscript.org@0.9.7"></script>
	</body>
</html>
)rawliteral";

const char footer_wallpaper[] PROGMEM = R"rawliteral(
		<div class="footer" %FOOTER_STYLE%>
            %UPDATE_NOTICE%
			<span>SQUiXL - 2025 Unexpected Maker - %FW_VER%</span><br>
			<span>Firmware Source available on <a href="https://github.com/squixl" target="_blank">on GitHUB</a></span>
			<!--<span>&nbsp;&nbsp;<a href="/debug_logs.html">debug logs</a></span>-->
		</div>
		<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz" crossorigin="anonymous"></script>
        <script src="https://unpkg.com/hyperscript.org@0.9.7"></script>
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

                // Set your max file size here (in bytes)
                const MAX_FILE_SIZE = 200 * 1024; // 200KB
                const MAX_WIDTH = 480;
                const MAX_HEIGHT = 480;

                let imgValid = false;
                let blockReload = false;

                const errorDiv = document.getElementById('file-error');
                const dimsDiv = document.getElementById('image-dimensions');
                const form = document.getElementById('uploadForm');
                const uploadBtn = document.getElementById('uploadBtn'); 


                document.getElementById('formFile').addEventListener('change', function(event) {
                    var file = event.target.files[0];

                    imgValid = false;
                    errorDiv.textContent = '';
                    dimsDiv.textContent = '';
                    uploadBtn.disabled = true;

                    if (!file) return;

                    // Check file size first (optional)
                    if (file.size > MAX_FILE_SIZE) {
                        errorDiv.textContent = `File is too large! Maximum allowed is ${MAX_FILE_SIZE/1024}KB.`;
                        form.reset();
                        return;
                    }

                    // Only allow .jpg/.jpeg
                    if (!file.name.match(/\.(jpg|jpeg)$/i)) {
                        errorDiv.textContent = "Please select a JPG image.";
                        form.reset();
                        return;
                    }

                    // Read the image dimensions
                    const reader = new FileReader();
                    reader.onload = function(e) {
                        const img = new Image();
                        img.onload = function() {
                            dimsDiv.textContent = `Selected image: ${img.width} x ${img.height} pixels.`;
                            if (img.width > MAX_WIDTH || img.height > MAX_HEIGHT) {
                                errorDiv.textContent = `Image must be at most ${MAX_WIDTH} x ${MAX_HEIGHT} pixels.`;
                                imgValid = false;
                                form.reset();
                                return;
                            } else {
                                imgValid = true;
                                uploadBtn.disabled = false;
                            }
                        };
                        img.onerror = function() {
                            errorDiv.textContent = "Cannot read image dimensions.";
                            imgValid = false;
                            form.reset();
                            return;
                        };
                        img.src = e.target.result;
                    };
                    reader.readAsDataURL(file);
                });

                // Prevent upload if not valid
                form.onsubmit = function(e) {
                    
                    var progressBar = document.getElementById('progress');
                    const fileInput = document.getElementById('formFile');
                    if (!fileInput.files.length) { // <--- Always check for file presence first
                        // REMOVE hx-post to prevent htmx request!
                        form.removeAttribute('hx-post');           // <--- KEY FIX!
                        e.preventDefault();
                        errorDiv.textContent = "Please select a file before uploading.";
                        dimsDiv.textContent = '';
                        form.reset();
                        uploadBtn.style.display = 'inline-block';
                        progressBar.style.display = 'none';
                        // Restore hx-post after a tick, so the user can try again
                        setTimeout(() => form.setAttribute('hx-post','/upload_wallpaper'), 10);
                        return false;
                    }
                    if (!imgValid) {
                        form.removeAttribute('hx-post');
                        e.preventDefault();
                        errorDiv.textContent ||= "Please select a valid image before uploading.";
                        form.reset();
                        blockReload = true;
                        setTimeout(() => form.setAttribute('hx-post','/upload_wallpaper'), 10);
                        return false;
                    }
                    // Valid: allow upload
                    uploadBtn.style.display = 'none';
                    progressBar.style.display = 'inline-block';
                    progressBar.value = 0;        
                };

			</script>
	</body>
</html>
)rawliteral";

const char meta[] PROGMEM = R"rawliteral(
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!--<link rel="icon" href="data:,">-->
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH" crossorigin="anonymous">
    <link rel="shortcut icon" href="https://reflowmasterpro.com/assets/images/favicon.png" type="image/png">
    <script src="https://code.jquery.com/jquery-3.7.1.min.js"   integrity="sha256-/JqT3SQfawRcv/BIHPThkBvs0OEvtFFmqPF/lYI/Cxo="   crossorigin="anonymous"></script>
    <script src="https://unpkg.com/htmx.org@2.0.4" integrity="sha384-HGfztofotfshcF7+8n44JQL2oJmowVChPTg48S+jvZoztPfvwD79OC/LTtG6dMp+" crossorigin="anonymous"></script>
    %CSS%
)rawliteral";

// const char css[] PROGMEM = R"rawliteral(
//     <style>
//     html {
//         background-color: #333;
//         height: 100%%;
//     }
//     body {
//         max-width: 90vw;
//         min-width: 80vw;
//         background-color: #fff;
//         margin: auto;
//         padding:10px;
//         font-size: 9pt;
//         min-height:100vh;
//         position:relative;
//         padding-bottom:80px;
//     }
//     h2 {
//       font-size: 16pt;
//     }
//     h3 {
//       font-size: 14pt;
//     }
//     h4 {
//       font-size: 12pt;
//     }
//     .error {
//       background: rgba(202, 60, 60, 0.2);
//     }
//     .yellow {
//       color: #ffc900;
//     }
//     .banner {
//       background: rgba(202, 60, 60, 0.8);
//       font-size: 16pt;
//       text-align:center;
//       color: #fff;
//     }
//     .banner_nice {
//       background: #fff;
//       font-size: 16pt;
//       text-align:center;
//       color: #333;
//     }

// 	.form_save.htmx-settling {
// 		opacity: 100;
// 	}

// 	.form_save {
// 		opacity: 0;
// 		transition: opacity 2s ease-out;
// 		color: #33ff33;
// 		font-size: 16pt;
// 	}

//     .alt {
//       background: #eee;
//     }
//     .alt2 {
//         background: #fff;
//     }
//     .heading {
//       background-color:#005cb5;
//       color:#fff;
//       padding:3px;
//     }
//     .right {
//       text-align:right;
//     }
//     .center {
//       text-align:center;
//     }
//     .cell_overflow {
//       overflow-x: hidden;
//       text-overflow: ellipsis;
//       width: 100%%;
//       white-space: nowrap;
//     }
//     .content {
//       max-width: 900px;
//     //   min-width: 900px;
//       margin: auto;
//     }
//     .no-wrap .nowrap {
//         display:inline-block;
//         white-space: nowrap;
//     }
//     .btn-pad-low {
//        padding:1px 4px 1px 4px;
//        font-size:9pt;
//     }
//     .footer {
//         position: absolute;
//         left: 0 ; right: 0; bottom: 0;
//         height:50px;
//         background-color: #005cb5;
//         color: #fff;
//         text-align:center;
// 		padding-top:10px;
//     }
//     .settings_heading {
//         font-size:14px;
//         background-color:#005cb5;
//         color:#fff;
//         margin-bottom:1px;
//         margin-left:10px;
//         padding:3px 10px 3px 10px;
//         border-radius: 5px 5px 0px 0px;
//     }

// 	.settings_info {
// 		font-size:13px;
// 		color:#bbb;
// 		text-align: center;
// 	}

//     .settings_frame {
//         border:1px #005cb5 solid;
//         padding:8px;
//         border-radius: 5px;
//         margin-bottom:20px;
//     }

// 	.settings_frame_inner {
//         border:1px #005cb5 solid;
//         background-color:#ddd;
//         padding:10px;
//         border-radius: 5px;
//         margin-bottom:20px;
//     }

//     .settings_reset_button {
//         border:2px #ff0000 solid;
//         background-color:#ff0000;
//         width:35px;
//         height:35px;
//         color:#fff;
//         text-align:center;
//         font-size:20px;
//     }

//     .settings_reset_button:hover {
//         background-color:#fff;
//         color:#ff0000;
//     }

//     .settings_reset_button_over {
//         border:2px #ff0000 solid;
//         background-color:#ff0000;
//         position: absolute;
//         right: 30px;
//         top: 7px;
//         padding: 4px;
//         width: 75px;
//         height: 26px;
//         cursor: pointer;
//         background-color:#ff0000;
//         vertical-align: middle;
//         font-size:12px;
//         text-align:center;
//         border-radius: 5px;
//         color:#fff;
//         display:none;
//     }

//     .settings_reset_button_over:hover {
//         background-color:#fff;
//         color:#ff0000;
//     }

//     .reset_field_div {
//         position: relative;
//         display: inline-block;
//     }

//     .table-row-back {
//         background-color:#fff;
//     }

//     .table-back {
//         background-color:#eee;
//         padding: 2px;
//         border: 1px solid 005cb5;
//     }

//     .table-row-header-above {
//         background-color:#005cb5;
//         color:#fff;
//         // border:1px solid #fff;
//     }

//     .table-row-header-aux {
//         background-color:#004466;
//         color:#fff;
//     }

//     .table-row-header {
//         border:0px;
//         background-color:#4a7caa;
//         color:#fff;
//         padding:0px;
//     }

//     .tablerow {
//         background-color:#eee;
//         border-bottom:1px solid #fff;
//         padding:0px;
//     }

//     .tablerow-alt {
//         background-color:#ddd;
//         border-bottom:1px solid #fff;
//         padding:0px;
//     }

//     .runtime-row {
//         height:40px;
//         font-size:24px;
//         color:#333;
//         font-weight:500;
//     }

//     .runtime-col {
//          color:#005cb5;
//     }

//     .revert {
//         border:2px solid #ff0000;
//     }

//     .save_info {
//         border:1px solid #bbb;
//         background-color:#dde;
//         padding:10px;
//         font-size:16px;
//         border-radius: 5px;
//     }

//     .chart_background {
//         background-color:#fff;
//     }

//     .flash {
//         -webkit-animation-name: flash-animation;
//         -webkit-animation-duration: 0.3s;

//         animation-name: flash-animation;
//         animation-duration: 0.3s;
//     }

//     @-webkit-keyframes flash-animation {
//         from { background: red; }
//         to   { background: default; }
//     }

//     @keyframes flash-animation {
//         from { background: red; }
//         to   { background: default; }
//     }

// 	.flash_post {
// 		font-size:16px;
// 		transition: background-color 0.5s ease-out;
// 	}

// </style>
// )rawliteral";

const char css_dark[] PROGMEM = R"rawliteral(
    <style>
    html {
        background-color: #111;
        height: 100%%;
    } 
    body {
        max-width: 90vw;
        min-width: 80vw;
        background-color: #222; 
        margin: auto;
        padding:10px;
        font-size: 9pt;
        min-height:100vh;
        position:relative; 
        padding-bottom:100px;
        color:#eee;
    }
    h2 {
      font-size: 16pt;
    }
    h3 {
      font-size: 14pt;
    }
    h4 {
      font-size: 12pt;
    }
    .error {
      background: rgba(202, 60, 60, 0.2);
    }
    .yellow {
      color: #ffc900;
    }
    .banner {
      background: rgba(202, 60, 60, 0.8);
      font-size: 16pt;
      text-align:center;
      color: #fff;
    }
    .banner_nice {
      background: #fff;
      font-size: 16pt;
      text-align:center;
      color: #333;
    }

	.form_save.htmx-settling {
		opacity: 100;
	}

	.form_save {
		// opacity: 0;
		transition: opacity 3s ease-out;
		color: #33ff33;
		font-size: 16pt;
	}

    .alt {
      background: #333;
    }

    .alt2 {
      background: #222;
    }
    
    .tablerow {
        background-color:#555;
        border-bottom:1px solid #222;
        padding:0px;
    }
    
    .tablerow-alt {
        background-color:#444;
        border-bottom:1px solid #222;
        padding:0px;
    }
    
    .heading {
      background-color:#005cb5;
      color:#fff;
      padding:3px;
    }
    .right {
      text-align:right;
    }
    .center {
      text-align:center;
    }
    .cell_overflow {
      overflow-x: hidden;
      text-overflow: ellipsis;
      width: 100%%;
      white-space: nowrap;
    }
    .content {
      max-width: 900px;
    //   min-width: 900px;
      margin: auto;
    }
    .no-wrap .nowrap {
        display:inline-block;
        white-space: nowrap;
    }
    .btn-pad-low {
       padding:1px 4px 1px 4px;
       font-size:9pt;
    }
    .footer {
        position: absolute; 
        left: 0 ; right: 0; bottom: 0; 
        height:50px;
        background-color: #005cb5;
        color: #fff;
        text-align:center;
		padding-top:10px;
    }

    .menu a {
        font-size:16px;
        font-weight:500;
    }

    .menu span {
        font-size:16px;
        font-weight:500;
        border-bottom:1px solid white;
    }

    .menu td {
        text-align:center;
        padding-left:10px;
        padding-right:10px;
    }
    
    .settings_heading {
        font-size:14px;
        background-color:#005cb5;
        color:#fff;
        margin-bottom:1px;
        margin-left:10px;
        padding:3px 10px 3px 10px;
        border-radius: 5px 5px 0px 0px;
    }

	.settings_info {
		font-size:13px;
		color:#bbb;
		text-align: center;
	}
    
    .settings_frame {
        border:1px #005cb5 solid;
        background-color:#333;
        padding:8px;
        border-radius: 5px;
        margin-bottom:20px;
    }

	.settings_frame_inner {
        border:1px #004ba4 solid;
        background-color:#222;
        padding:10px;
        border-radius: 5px;
        margin-bottom:20px;
    }
    
    .settings_reset_button {
        border:2px #ff0000 solid;
        background-color:#ff0000;
        width:35px;
        height:35px;
        color:#fff;
        text-align:center;
        font-size:20px;
    }
    
    .settings_reset_button:hover {
        background-color:#fff;
        color:#ff0000;
    }
    
    .settings_reset_button_over {
        border:2px #ff0000 solid;
        background-color:#ff0000;
        position: absolute;
        right: 30px;
        top: 7px;
        padding: 4px;
        width: 75px;
        height: 26px;
        cursor: pointer;
        background-color:#ff0000;
        vertical-align: middle;
        font-size:12px;
        text-align:center;
        border-radius: 5px;
        color:#fff;
        display:none;
    }
    
    .settings_reset_button_over:hover {
        background-color:#fff;
        color:#ff0000;
    }
    
    .reset_field_div {
        position: relative;
        display: inline-block;
    }

    td {
        color:#eee;
    }

    .table-row-back {
        background-color:#222;
    }

    .table-back {
        background-color:#333;
        padding: 2px;
        border: 1px solid 005cb5;
    }
    
    .table-row-header-above {
        background-color:#005cb5;
        color:#fff;
        // border:1px solid #222;
        //width:13%%;
    }

    .table-row-header-aux {
        background-color:#004466;
        color:#fff;
    }
    
    .table-row-header {
        border:0px;
        background-color:#4a7caa;
        color:#fff;
        padding:0px;
        //width:13%%;
    }
    
    .runtime-row {
        height:40px;
        font-size:24px;
        color:#aaa;
        font-weight:500;
    }
    
    .runtime-col {
         color:#006ad1;
    }
    
    .revert {
        border:2px solid #ff0000;

    }
    
    .save_info {
        border:1px solid #000;
        background-color:#111;
        padding:10px;
        font-size:16px;
        border-radius: 5px;
    }

    .flash {
        -webkit-animation-name: flash-animation;
        -webkit-animation-duration: 0.3s;

        animation-name: flash-animation;
        animation-duration: 0.3s;
    }

    @-webkit-keyframes flash-animation {  
        from { background: red; }
        to   { background: default; }
    }

    @keyframes flash-animation {  
        from { background: red; }
        to   { background: default; }
    }

	.flash_post {
		// background-color: green;
		font-size:16px;
		transition: background-color 0.5s ease-out;
	}

</style>
)rawliteral";