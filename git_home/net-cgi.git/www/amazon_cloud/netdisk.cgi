<% http_header("style/form2.css") %>
<body onLoad=loadvalue();>
<style>
.err_block{background-color:#F7D9D7;width:450px;height:100px; margin-left:150px;margin-top:150px}
.err_msg{padding-top:40px; color:#CA5137;font-size:16;}
</style>
<script>
var cloud_url="<% cat_file("/etc/drive_login_link") %>";
var local_url="<% cfg_get("cloud_url") %>";

function goto_newurl()
{
	var newurl= local_url+'netdisk_scan.htm';
	top.location.href=newurl;
}

function try_again()
{
	top.location.href=cloud_url;
}

function loadvalue()
{
	var hrefstr = window.location.href;
	var pos = hrefstr.indexOf("code=");
	if( hrefstr.indexOf("access_denied") > 0){
		try_again();
	}
	else if(pos > 0)
	{
		document.getElementById("error_div").style.display="none";
		document.getElementById("auth_div").style.display="";
		setTimeout("goto_newurl();",3000);
	}
	else
	{
		document.getElementById("error_div").style.display="";
		document.getElementById("auth_div").style.display="none";
		setTimeout("try_again();",3000);
	}
}
</script>

<div id="error_div" style="display:none">
<div class="big-corner-all err_block" align="center">
<div class="err_msg">Oops, we could not find an account with the email and password you've entered.</div>
</div>
</div>

<div id="auth_div" style="display:none">
<div  align="center"><BR><BR><B>Authenticating...</B><BR><BR></div>
<div class="waiting_img_div" align="center"><img src="image/wait30.gif" /></div>
</div>
</body>
</html>
