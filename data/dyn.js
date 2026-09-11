// Copyright (c) 2025 steff393, MIT license

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
			element.innerHTML = values[key];
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

const providerNames = ['aWATTar (DE)', 'aWATTar (AT)', 'Tibber'];
const weekdayNames   = ['So', 'Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa'];
const dynModeButtons = document.querySelectorAll('[data-dyn-mode]');

let currentSlots  = [];
let chartScrolled = false;

function pad(n) { return (n < 10 ? '0' : '') + n; }

function fmtTime(unixSec) {
	if (!unixSec) { return '-'; }
	const d = new Date(unixSec * 1000);
	return pad(d.getDate()) + '.' + pad(d.getMonth() + 1) + '.' + d.getFullYear() + ' - ' + pad(d.getHours()) + ':' + pad(d.getMinutes());
}

function fmtHM(unixSec) {
	const d = new Date(unixSec * 1000);
	return pad(d.getHours()) + ':' + pad(d.getMinutes());
}

// find the charge-window (run of consecutive charge slots) that contains or follows "now"
function findWindow(slots, now) {
	const nowIdx = slots.findIndex(s => now >= s[0] && now < s[0] + 3600);

	function runEnd(i) {
		let end = slots[i][0] + 3600;
		let j = i + 1;
		while (j < slots.length && slots[j][2] === 1 && slots[j][0] === end) { end += 3600; j++; }
		return end;
	}

	if (nowIdx >= 0 && slots[nowIdx][2] === 1) {
		return { active: true, until: runEnd(nowIdx) };
	}
	for (let i = Math.max(nowIdx + 1, 0); i < slots.length; i++) {
		if (slots[i][2] === 1 && slots[i][0] >= now) {
			return { active: false, from: slots[i][0], until: runEnd(i) };
		}
	}
	return null;
}

function windowText(slots, now) {
	if (slots.length === 0) { return 'warte auf Preisdaten'; }
	const w = findWindow(slots, now);
	if (!w) { return 'kein Ladefenster in den vorliegenden Preisdaten'; }
	if (w.active) { return 'lädt noch bis ' + fmtHM(w.until) + ' Uhr'; }
	return 'ab ' + fmtHM(w.from) + ' bis ' + fmtHM(w.until) + ' Uhr';
}

function avgStatsText(slots) {
	if (slots.length === 0) { return ''; }
	const all   = slots.map(s => s[1] / 10);
	const cheap = slots.filter(s => s[2] === 1).map(s => s[1] / 10);
	const avgAll = all.reduce((a, b) => a + b, 0) / all.length;
	if (cheap.length === 0) { return 'Ø ' + avgAll.toFixed(1) + ' ct/kWh'; }
	const avgCheap = cheap.reduce((a, b) => a + b, 0) / cheap.length;
	return 'Ø ' + avgAll.toFixed(1) + ' ct/kWh &middot; in Ladefenstern Ø ' + avgCheap.toFixed(1) + ' ct/kWh';
}

function savingText(slots) {
	const all   = slots.map(s => s[1] / 10);
	const cheap = slots.filter(s => s[2] === 1).map(s => s[1] / 10);
	if (all.length === 0 || cheap.length === 0) { return ''; }
	const avgAll   = all.reduce((a, b) => a + b, 0) / all.length;
	const avgCheap = cheap.reduce((a, b) => a + b, 0) / cheap.length;
	const saving = avgAll > 0 ? Math.round((1 - avgCheap / avgAll) * 100) : 0;
	return saving > 0 ? 'Einsparung: -' + saving + '%' : '';
}

function selectSlot(idx) {
	const svg = document.getElementById('chartSvg');
	if (svg) {
		svg.querySelectorAll('.bar-selected').forEach(el => el.classList.remove('bar-selected'));
	}
	const info = document.getElementById('chartSelInfo');
	if (!info) { return; }
	if (idx === null || idx < 0 || !currentSlots[idx]) {
		info.textContent = currentSlots.length ? 'Balken antippen für Details' : '';
		return;
	}
	const s = currentSlots[idx];
	const bar = svg && svg.querySelector('.bar[data-idx="' + idx + '"] rect.bar-fill');
	if (bar) { bar.classList.add('bar-selected'); }
	info.textContent = fmtHM(s[0]) + '–' + fmtHM(s[0] + 3600) + ' Uhr: ' + (s[1] / 10).toFixed(1) + ' ct/kWh' + (s[2] === 1 ? ' · günstiges Fenster' : '');
}

function drawChart(data) {
	const wrap  = document.getElementById('chartWrap');
	const slots = data.slots || [];
	currentSlots = slots;

	if (slots.length === 0) {
		wrap.innerHTML = '<p style="font-weight:normal">Noch keine Preisdaten geladen.</p>';
		selectSlot(null);
		return;
	}

	const padding = { top: 14, right: 12, bottom: 26, left: 32 };
	const barW    = 14;
	const gap     = 4;
	const chartH  = 160;
	const chartInnerW = slots.length * (barW + gap) - gap;
	const width   = chartInnerW + padding.left + padding.right;
	const height  = chartH + padding.top + padding.bottom;

	const prices   = slots.map(s => s[1] / 10);
	const minPrice = Math.min(0, ...prices);   // y-axis always starts at 0 ct/kWh (extends further down only for negative prices)
	const maxPrice = Math.max(...prices);
	const range    = Math.max(1, maxPrice - minPrice);
	const y = v => padding.top + chartH - ((v - minPrice) / range) * chartH;
	const x = i => padding.left + i * (barW + gap);
	const now = data.now || 0;

	let nowIdx = -1;
	slots.forEach((s, i) => { if (now >= s[0] && now < s[0] + 3600) { nowIdx = i; } });

	let svg = '<svg id="chartSvg" class="chart-svg" viewBox="0 0 ' + width + ' ' + height + '" width="' + width + '" height="' + height + '">';
	svg += '<rect x="0" y="0" width="' + width + '" height="' + height + '" rx="8" fill="rgba(0,0,0,0.03)"/>';

	// y gridlines with price labels
	for (let step = 0; step <= 2; step++) {
		const v    = maxPrice - (range / 2) * step;
		const yPos = padding.top + (chartH / 2) * step;
		svg += '<line x1="' + padding.left + '" y1="' + yPos.toFixed(1) + '" x2="' + (padding.left + chartInnerW) + '" y2="' + yPos.toFixed(1) + '" stroke="rgba(0,0,0,0.12)" stroke-width="0.8"/>';
		svg += '<text x="' + (padding.left - 4) + '" y="' + (yPos + 3).toFixed(1) + '" font-size="8.5" fill="#333" text-anchor="end">' + v.toFixed(1) + '</text>';
	}

	// day dividers at local midnight
	let lastDateKey = null;
	for (let i = 0; i < slots.length; i++) {
		const d = new Date(slots[i][0] * 1000);
		const dateKey = d.getFullYear() + '-' + d.getMonth() + '-' + d.getDate();
		if (lastDateKey !== null && dateKey !== lastDateKey) {
			const lx = (x(i) - gap / 2).toFixed(1);
			svg += '<line x1="' + lx + '" y1="' + padding.top + '" x2="' + lx + '" y2="' + (padding.top + chartH) + '" stroke="rgba(0,0,0,0.35)" stroke-width="1" stroke-dasharray="2,2"/>';
			svg += '<text x="' + (x(i) + 2).toFixed(1) + '" y="' + (padding.top + 8) + '" font-size="8.5" fill="#333">' + weekdayNames[d.getDay()] + '</text>';
		}
		lastDateKey = dateKey;
	}

	for (let i = 0; i < slots.length; i++) {
		const start = slots[i][0];
		const p = prices[i];
		const isCharge = slots[i][2] === 1;
		const px = x(i);
		const py = y(p);
		const barTop = Math.min(py, padding.top + chartH);
		const barH   = Math.max(2, Math.abs(py - (padding.top + chartH)));
		const fill   = isCharge ? 'var(--theme-color-2)' : 'var(--theme-color-5)';

		svg += '<g class="bar" data-idx="' + i + '">';
		svg += '<rect class="bar-hit" x="' + (px - gap / 2).toFixed(1) + '" y="' + padding.top + '" width="' + (barW + gap) + '" height="' + chartH + '" fill="transparent"/>';
		svg += '<rect class="bar-fill" x="' + px.toFixed(1) + '" y="' + barTop.toFixed(1) + '" width="' + barW + '" height="' + barH.toFixed(1) + '" fill="' + fill + '"/>';
		if (i === nowIdx) {
			svg += '<polygon points="' + (px + barW / 2 - 4).toFixed(1) + ',' + (padding.top - 3) + ' ' + (px + barW / 2 + 4).toFixed(1) + ',' + (padding.top - 3) + ' ' + (px + barW / 2).toFixed(1) + ',' + (padding.top + 3) + '" fill="var(--headline-1-color)"/>';
		}
		svg += '<title>' + fmtHM(start) + '–' + fmtHM(start + 3600) + ' Uhr: ' + p.toFixed(1) + ' ct/kWh' + (isCharge ? ' (Ladefenster)' : '') + '</title>';
		svg += '</g>';

		const hour = new Date(start * 1000).getHours();
		if (hour % 3 === 0) {
			svg += '<text x="' + (px + barW / 2).toFixed(1) + '" y="' + (height - 8) + '" font-size="8.5" fill="#333" text-anchor="middle">' + pad(hour) + '</text>';
		}
	}
	svg += '</svg>';
	wrap.innerHTML = svg;

	wrap.querySelectorAll('.bar').forEach(el => {
		el.addEventListener('click', () => selectSlot(parseInt(el.getAttribute('data-idx'), 10)));
	});

	selectSlot(nowIdx);

	// on first load, scroll so "now" is roughly centered - the chart can be wider than the screen
	if (!chartScrolled && nowIdx >= 0) {
		chartScrolled = true;
		wrap.scrollLeft = Math.max(0, x(nowIdx) - wrap.clientWidth / 2 + barW / 2);
	}
}

function render(data) {
	// mode buttons
	for (const element of dynModeButtons) {
		setClass(element, 'active', data.mode === parseInt(element.getAttribute('data-dyn-mode')));
	}

	// license warning (only relevant once the feature is actually switched on)
	setSectionVisibility('licWarn', data.lic === 0 && data.mode >= 1);

	// status text
	let state = 'nicht konfiguriert';
	if (data.mode === 2) {
		if (data.charging)   { state = 'lädt (günstiges Fenster)'; }
		else if (data.valid) { state = 'wartet auf günstiges Fenster'; }
		else                 { state = 'warte auf Preisdaten'; }
	} else if (data.mode === 1) {
		state = 'aus';
	}

	// strategy text
	let strat;
	if (data.stratMode === 1) {
		strat = 'Preis ≤ ' + (data.maxPrice / 10).toFixed(1) + ' ct/kWh';
	} else {
		strat = 'günstigste ' + data.hours + ' Std.';
	}
	strat += ' &middot; ' + (providerNames[data.provider] || '?');
	if (data.markup) {
		strat += ' &middot; +' + (data.markup / 10).toFixed(1) + ' ct Aufschlag';
	}

	assignValuesToHtml({
		dynState:   state,
		dynPrice:   (data.price !== undefined && data.price !== null) ? (data.price / 10).toFixed(1) : '-',
		dynStrat:   strat,
		dynWindow:  windowText(data.slots || [], data.now || 0),
		dynAvg:     avgStatsText(data.slots || []),
		dynSaving:  savingText(data.slots || []),
		dynUpdated: fmtTime(data.updated),
	});

	drawChart(data);
}

function refresh() {
	fetch('/dyn')
		.then(response => response.json())
		.then(render)
		.catch(error => console.log('fetch /dyn failed: ', error));
}

function setMode(mode) {
	fetch('/dyn?dynMode=' + mode)
		.then(response => response.json())
		.then(render)
		.catch(error => console.log('set dynMode failed: ', error));
}

window.addEventListener('DOMContentLoaded', () => {
	initNavBar();
	setSectionVisibility('licWarn', false);
	for (const element of dynModeButtons) {
		element.addEventListener('click', () => setMode(element.getAttribute('data-dyn-mode')));
	}
	refresh();
	setInterval(refresh, 30000);
});
