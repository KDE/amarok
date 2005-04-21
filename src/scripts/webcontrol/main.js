function push() {
  var sub = this.length;
  for (var i = 0; i < push.arguments.length; ++i) {
    this[sub] = push.arguments[i];
        sub++;
  }
}

function pop() {
  var lastElement = null;
  if (this.length > 0) {
          lastElement = this[this.length - 1];
	  this.length--;
  }
  return lastElement;
}

Array.prototype.push = push;
Array.prototype.pop = pop;

function getById(id) {
  if (document.layers){	
    //Netscape 4 specific code
    pre = 'document.';
    post = '';
  }
  if (document.getElementById){
    //Netscape 6 specific code
    pre = 'document.getElementById("';
    post = '")';
  } else
  if (document.all){
    //IE4+ specific code
    pre = 'document.all.';
    post = '';
  }
  return eval(pre + id + post);
}

// All this stuff because we want it to be reentrant
var animScriptArr = new Object();
var nextAnimId = 0;
var animate_speed = 90;
function animScriptsHelper(index, delay)
{
	if (animScriptArr[index] == null) {
		return;
	}
	script = animScriptArr[index].pop();
	if (script != null) {
		eval(script);
	}	
	if (animScriptArr[index].length > 1) {
		setTimeout("animScriptsHelper(" + index + ", " + delay + ");", delay);
	} else {
		if (animScriptArr[index].length == 1) {
			script = animScriptArr[index].pop();
			delete animScriptArr[index];
			if (script != null) {
				eval(script);
			}
		} else {
			delete animScriptArr[index];
		}	
	}	
}

function setBackgroundColor(r,g,b,rowid)
{
	getById(rowid).style.background = "rgb(" + r + "," + g + "," + b + ")";
}

function setColor(r,g,b, myid)
{
	var i, td_n_elems, td_elems = getById(myid).getElementsByTagName("a");
	td_n_elems = td_elems.length;

	for (i = 0; i < td_n_elems; i++) {
		td_elems[i].style.color = "rgb(" + r + "," + g + "," + b + ")";
	}

}

function anim(direction)
{

	// build color fade
	var ci;
	var count = 20;
	var mult = Math.floor(256 / count);
	var colorAnimScripts = new Array();
	var cid = 0;
	var cid2 = 0;
	var animid = "nowplaying";

	if (direction == 1) {
		colorAnimScripts[0] = "anim(0);";
	} else {	
		colorAnimScripts[0] = "anim(1);";
	}

	for (ci = 1; ci < count+1; ci++) {
		if (direction) {
			cid = count - (ci-1);	
			cid2 = (ci-1);	
		} else {
			cid = (ci-1);
			cid2 = count - (ci-1);	
		}
		colorAnimScripts[ci] = 
			"setBackgroundColor(" + Math.floor(13.0 * (cid*mult/100.0) + 100.0) + "," 
			+ Math.floor(13.0 * (cid*mult/100.0) + 100.0) + "," 
			+ Math.floor(13.0 * (cid*mult/100.0) + 140.0) + ", '" + animid + "');";
	}

	setColor(255,255,0, "nowplaying");
	animScriptArr[0] = colorAnimScripts;
	animScriptsHelper(0, animate_speed+0);
//	animScriptArr[nextAnimId++] = colorAnimScripts;
//	animScriptsHelper(nextAnimId-1, animate_speed+0);
}


var dateobj = -1;
var dhours = 0;
var dmins = 0;
var dsecs = 0;
function countdown(value) {
	
	// all this is done because setTimeout 1000, isn't really a second in js

	if (dateobj == -1) {
		dateobj = new Date();
		dhours = dateobj.getUTCHours();
		dmins = dateobj.getUTCMinutes();
		dsecs = dateobj.getUTCSeconds();

		// refresh automatically every 20 secs.
		setTimeout("dateobj=-1;refreshPage();", 20000);
	}

	dateobj2 = new Date();
	var hours_now = dateobj2.getUTCHours();
	var mins_now = dateobj2.getUTCMinutes();
	var secs_now = dateobj2.getUTCSeconds();

	// difference (assume no number is more that an hour :)
	
	msecs = ((hours_now == dhours ? 0 : 1) * 60 * 60 +
		(mins_now - dmins) * 60 +
		(secs_now - dsecs)) * 1000;

	diff = value * 1000 - msecs;
	tdo = new Date(diff);

	rhours = tdo.getUTCHours();
	rmins = tdo.getUTCMinutes();
	rsecs = tdo.getUTCSeconds();

	var hours_str = (rhours == 0 ? "" : (rhours < 10 ? "0" + rhours : rhours) + ":");
	var mins_str = rmins < 10 ? "0" + rmins : rmins;
	var secs_str = rsecs < 10 ? "0" + rsecs : rsecs;

	getById("countdown").innerHTML = hours_str + mins_str + ":" + secs_str;

	setHeaders();

	if (diff > 0) {
		setTimeout("countdown(" + value + ");", 500);
	} else {
		getById("countdown").innerHTML = "00:00";
		// go refresh this page
		refreshPage();
	}
}

function getPageScroll()
{
	var x,y;
	if (self.pageYOffset) // all except Explorer
	{
	        x = self.pageXOffset;
	        y = self.pageYOffset;
	}
	else if (document.documentElement && document.documentElement.scrollTop)
	        // Explorer 6 Strict
	{
	        x = document.documentElement.scrollLeft;
	        y = document.documentElement.scrollTop;
	}
	else if (document.body) // all other Explorers
	{
	        x = document.body.scrollLeft;
	        y = document.body.scrollTop;
	}
	return y;
}

function setPageScroll(pxs)
{
	window.scrollTo(0, pxs);
}

function setScrollInUrl(newUrl)
{

	if (newUrl.match(/scroll=\d*/)) {
		return newUrl.replace(/scroll=\d*/g, "scroll=" + getPageScroll());
	} else {
		if (newUrl.match(/\?/)) {
			return newUrl + "&scroll=" + getPageScroll();
		} else {
			return newUrl + "?scroll=" + getPageScroll();
		}
	}
}
	
function refreshPage() 
{
	document.location = setScrollInUrl("" + document.location);
}

function rescroll() 
{
	regex = new RegExp("scroll=[^\\&]+");
	var match = regex.exec(document.URL);
	scrollPos = parseInt((match+"").substring(7));
	if (match) {
		setPageScroll(scrollPos);
	}
}

function dolink(qstr)
{
	document.location = setScrollInUrl("?" + qstr);
}

function setHeaders()
{
	var i, td_n_elems_dest, td_elems_dest = getById("trackheader").getElementsByTagName("th");
	td_n_elems = td_elems_dest.length;

	var td_n_elems, td_elems;
	if (getById("trackone")) {
		td_n_elems, td_elems = getById("trackone").getElementsByTagName("td");
	} else {
		td_n_elems, td_elems = getById("nowplaying").getElementsByTagName("td");
	}

	for (i = 0; i < td_n_elems; i++) {
		td_elems_dest[i].style.width = td_elems[i].offsetWidth;
	}
}