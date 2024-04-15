// Copyright (c) 2024 steff393, MIT license

// ----------------------------- COMMON SECTION: START ------------------------------
function initNavBar() {
	for (const element of document.querySelectorAll('[top-nav-link]')) {
		element.addEventListener('click', function() {window.location.href = element.getAttribute('top-nav-link')});
	}
}

function assignValuesToHtml(values) {
	let valueContainerElements = document.querySelectorAll('[data-value]');
	for (const element of valueContainerElements) {
		const key = element.getAttribute('data-value');
		if (values[key] !== undefined) {
			element.innerHTML = values[key].toLocaleString('de-DE');
		}
	}
}

function setClass(element, className, state) {
	if (state) {
		element.classList.add(className)
	} else {
		element.classList.remove(className)
	}
}

function setSectionVisibility(sectionId, isVisible) {
	setClass(document.getElementById(sectionId), 'not-available', !isVisible);
}
// ----------------------------- COMMON SECTION:  END  ------------------------------

window.addEventListener('DOMContentLoaded', () => {
	// Adjustable values -----
	let phases         = 3;   // number of phases
	// -----------------------
	let Socket;
	let elementCurrentSlider   = document.getElementById('slideCurr');
	let pvModeButtons          = document.querySelectorAll('[data-pv-mode]');
	let wallboxButtons         = document.querySelectorAll('[data-wallbox-id]');
	let valueContainerElements = document.querySelectorAll('[data-value]');

	let sliderSliding = false;

	function init() {
		setSectionVisibility('enwg14a',    false);
		setSectionVisibility('connection', false);
		setSectionVisibility('boxSelection', wallboxButtons.length > 1);
		setSectionVisibility('pvLaden', false);
		initNavBar();
		document.getElementById('btnExit').addEventListener('click', exit);

		Socket = new WebSocket(`ws://${window.location.hostname}:81/`);
		Socket.onmessage = processReceivedCommand;

		// Update the current slider value (each time you drag the slider handle)
		elementCurrentSlider.addEventListener('input', onStartSliderSliding);
		elementCurrentSlider.addEventListener('change', onSliderReleased);

		for (const element of document.querySelectorAll('[data-send-command]')) {
			element.addEventListener('click', () => {
				sendText(element.getAttribute('data-send-command'));
			})
		}
	}
	
	function onStartSliderSliding() {
		sliderSliding = true;

		let val = parseInt(elementCurrentSlider.value);
		if ((val >= 0) && (val <= 30)) {
			elementCurrentSlider.value = val = 0;
		}
		if ((val > 30) && (val <= 60)) {
			elementCurrentSlider.value = val = 60;
		}

		assignValuesToHtml({
			currLim:  val / 10,
		})
	}

	function onSliderReleased() {
		let val = parseInt(elementCurrentSlider.value);
		if ((val >= 0) && (val <= 30)) {
			elementCurrentSlider.value = val = 0;
		}
		if ((val > 30) && (val <= 60)) {
			elementCurrentSlider.value = val = 60;
		}
			

		sendText(`currLim=${val}`);
		sliderSliding = false;
	}

	function setClass(element, className, state) {
		if (state) {
			element.classList.add(className)
		} else {
			element.classList.remove(className)
		}
	}

	function processReceivedCommand(evt) {
		const message = JSON.parse(evt.data);
		let carStat;
		let wbStat;
		switch (message.chgStat) {
			case  2: /*A1*/ carStat = 'nein'; wbStat = 'nein'; break;
			case  3: /*A2*/ carStat = 'nein'; wbStat = 'ja'; break;
			case  4: /*B1*/ carStat = 'ja, ohne Ladeanf.'; wbStat = 'nein'; break;
			case  5: /*B2*/ carStat = 'ja, ohne Ladeanf.'; wbStat = 'ja'; break;
			case  6: /*C1*/ carStat = 'ja,  mit Ladeanf.'; wbStat = 'nein'; break;
			case  7: /*C2*/ carStat = 'ja,  mit Ladeanf.'; wbStat = 'ja'; break;
			default: carStat = message.chgStat; wbStat = '-';
		}
		assignValuesToHtml({
			carStat: carStat,
			wbStat:  wbStat,
			power:   message.power / 1000,
			energyI: message.energyI,
			energyC: message.energyC,
			watt:    message.watt / 1000,
			enwgErr: message.enwgErr ? "Fehler in der Funktion §14a EnWG. <br>Die Anlage ist nicht mehr EnWG-konform (" + message.enwgErr + "). " : "",
			timeNow: message.timeNow,
		})

		if (!sliderSliding) {
			assignValuesToHtml({
				currLim:  message.currLim,
			});
			elementCurrentSlider.value = message.currLim * 10;
		}

		for (const element of pvModeButtons) {
			setClass(element, 'active', message.pvMode === parseInt(element.getAttribute('data-pv-mode')));
		}
		setSectionVisibility('enwg14a',    message.enwg14a ==  1);
		setSectionVisibility('connection', message.failCnt >= 10);
		setSectionVisibility('pvLaden', message.pvMode >= 1 && message.pvMode <= 3);

		for (const element of wallboxButtons) {
			setClass(element, 'active', message.id === parseInt(element.getAttribute('data-wallbox-id')));
		}
	}

	function exit() {
		assignValuesToHtml({
			carStat: '-',
			wbStat:  '-',
			power:   '-',
			energyI: '-',
			energyC: '-',
			currLim: '-',
			watt:    '-',
			timeNow: '-',
		})
		for (const element of document.querySelectorAll('[data-wallbox-id],[data-pv-mode]')) {
			setClass(element, 'active', false);
			setClass(element, 'disabled', true);
		}
		Socket.close();
	}

	function sendText(data){
		Socket.send(data);
	}

	init();

});
