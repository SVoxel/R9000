function LTNS() {
	function fun_cancelBubble(yq) {
		if (!yq) {
			return;
		};
		if (yq.stopPropagation) {
			yq.preventDefault();
			yq.stopPropagation();
		} else if (document.all) {
			yq.cancelBubble = true;
			yq.returnValue = false;
		};
	};

	function fun_addListener(element, event, func, iq) {
		var oq = [element, event];
		var pq = (element.tagName || element == window || element == document);
		if (pq) {
			if (element.addEventListener) {
				element.addEventListener(event, func, false);
			} else if (element.attachEvent) {
				element.attachEvent("on" + event, func);
			} else {
				var aq = element["on" + event];
				if (typeof(aq) == "function") {
					element["on" + event] = function () {
						aq();
						func();
					};
				} else {
					element["on" + event] = func;
				};
			};
		};
	};
	
	var a = {cancelBubble:fun_cancelBubble, addListener:fun_addListener};
	var obj = {LTEvent:a};
	for(var val in obj)
	{
		window[val] = obj[val];
	}
};
LTNS();