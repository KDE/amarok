function preloadImages(sources)
{
	var image;
	for (var i = 0; i < sources.length; i++) {
		image = new Image();
		image.src = sources[i];
	}
}

preloadImages([
	"icons/wait.png",
	"icons/new.png",
	"icons/confirmed.png",
	"icons/progress.png",
	"icons/solved.png",
	"icons/invalid.png",
	"icons/duplicate.png",
	"icons/id.png"
]);

/**
 * Note: This part has been done very quickly in a few hours.
 * It will be AJAX-powered in the future.
 */
function showStatusMenu(event)
{
	// Compatible with IE:
	if (window.event) {
		event = window.event;
		event.target = event.srcElement;
	}

	// Initialize the menu links:
	var clickedId = event.target.id;
	clickedId = clickedId.substring(15, clickedId.length); // The img id is of the form "status_comment_###". We want the "###"
//	alert("cid" + clickedId);
	// TODO: addClass="clickedPopup___"

	//
	document.getElementById("markAsNew").href       = "?useSessionFilter=true&markAs=New&id="       + clickedId;
	document.getElementById("markAsConfirmed").href = "?useSessionFilter=true&markAs=Confirmed&id=" + clickedId;
	document.getElementById("markAsProgress").href  = "?useSessionFilter=true&markAs=Progress&id="  + clickedId;
	document.getElementById("markAsSolved").href    = "?useSessionFilter=true&markAs=Solved&id="    + clickedId;
	document.getElementById("markAsInvalid").href   = "?useSessionFilter=true&markAs=Invalid&id="   + clickedId;

	document.getElementById("markAsNew").onclick       = function(event) { return changeState(clickedId, "New");       };
	document.getElementById("markAsConfirmed").onclick = function(event) { return changeState(clickedId, "Confirmed"); };
	document.getElementById("markAsProgress").onclick  = function(event) { return changeState(clickedId, "Progress");  };
	document.getElementById("markAsSolved").onclick    = function(event) { return changeState(clickedId, "Solved");    };
	document.getElementById("markAsInvalid").onclick   = function(event) { return changeState(clickedId, "Invalid");   };

	// Show the menu:
	var icon = event.target;
//	alert("i" + icon);
	var menu = document.getElementById("statusMenu");
//	alert("m" + menu);
	menu.style.top  = icon.y + icon.height + 3;
//	alert("ok?");
	menu.style.left = icon.x - 1;
//	alert("encore?");
	menu.style.display = "block";
//	alert("fin");
	//document.getElementById("markAsNew").focus();

	// Do not follow the href link:
	return false;
}

function changeState(id, state)
{
	var followLink = true;
	var request = new XMLHttpRequest();
	if (request) {
		var tr    = document.getElementById("comment_" + id);
		var image = document.getElementById("status_comment_" + id);
		image.src = "icons/wait.png";
		request.onreadystatechange = function() {
//			alert(request.readyState + "    " + request.status);
			if (request.readyState == 4 && (request.status == 200 || request.status == 304)) {
				// Change the state:
				image.src = "icons/" + state.toLowerCase() + ".png";
				tr.removeClass("New");
				tr.removeClass("Confirmed");
				tr.removeClass("Progress");
				tr.removeClass("Solved");
				tr.removeClass("Invalid");
				tr.addClass(state);
				if (!shownStatus[state]) {
					tr.style.display = "none";
					document.getElementById("commentCount").innerHTML -= 1;
				}
				// Hide the menu:
				document.getElementById("statusMenu").style.display = "none";
			}
		};
		request.open("GET", "view.php?useSessionFilter=true&isAJAX=true&markAs=" + state + "&id=" + id, true);
		request.send(null/*"useSessionFilter=true&isAJAX=true&markAs=" + state + "&id=" + id*/);
		followLink = false;
	}
	return followLink;
}

function hideStatusMenu(event)
{
	// Compatible with IE:
	if (window.event)
		event = window.event;

	// If the target is a child of statusMenu, then user clicked the menu: do not hide it:
	for (var clicked = event.target; clicked; clicked = clicked.parentNode)
		if (clicked.id == "statusMenu")
			return true;

	// Hide the menu:
	document.getElementById("statusMenu").style.display = "none";
	return false;
}
