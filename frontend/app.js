// ═══════════════════════════════════════════════════
//  آدرس ESP32 — اینجا IP یا دامنه ESP32 رو بذار
//  مثال: 'http://192.168.1.50'  یا  'http://esp32.local'
// ═══════════════════════════════════════════════════
const ESP32_URL = 'http://192.168.1.50';

// ── State ─────────────────────────────────────────────────
let DEVICES = [];
let currentCat = 'tv';
let currentBrand = 'all';
let openDeviceId = null;
let learnModeOn = false;
let learnPolling = null;
let learnCdInterval = null;
let pendingLearnBtn = null;
let learnQueue = [];
let learnQueueIdx = 0;
let addDeviceData = {};
const selectedAddBtns = new Set();

// ── دکمه‌های پیش‌فرض برای add device ────────────────────
const PRESET_BTNS = [
  {name:"Power",icon:"⏻"}, {name:"Vol+",icon:"🔊"}, {name:"Vol-",icon:"🔉"},
  {name:"Mute",icon:"🔇"}, {name:"CH+",icon:"⬆"},  {name:"CH-",icon:"⬇"},
  {name:"OK",icon:"✔"},   {name:"Up",icon:"▲"},    {name:"Down",icon:"▼"},
  {name:"Left",icon:"◀"}, {name:"Right",icon:"▶"}, {name:"Back",icon:"↩"},
  {name:"Home",icon:"🏠"}, {name:"Menu",icon:"☰"}, {name:"Source",icon:"⎘"},
  {name:"Cool",icon:"❄"},  {name:"Heat",icon:"🔥"}, {name:"Fan",icon:"💨"},
  {name:"Temp+",icon:"🌡"},{name:"Temp-",icon:"🌡"},
  {name:"Play",icon:"▶"},  {name:"Pause",icon:"⏸"},{name:"Stop",icon:"⏹"},
];

// ── آیکون دسته ──────────────────────────────────────────
function catIcon(cat, label) {
  if (cat === 'tv') return '📺';
  if (cat === 'ac') return '❄️';
  if (label === 'پروژکتور') return '📽️';
  if (label === 'ست‌تاپ باکس') return '📡';
  if (label === 'DVD Player') return '💿';
  if (label === 'A/V Receiver') return '🔊';
  if (label === 'پنکه') return '💨';
  return '📱';
}

// ── بارگذاری دستگاه‌ها ───────────────────────────────────
async function loadDevices() {
  try {
    const r = await fetch(ESP32_URL + '/devices');
    DEVICES = await r.json();
    renderAll();
  } catch(e) {
    DEVICES = [];
    renderAll();
    showToast('⚠️ اتصال به ESP32 برقرار نشد', true);
  }
}

// ── رندر کلی ────────────────────────────────────────────
function renderAll() {
  renderBrandBar();
  renderGrid();
}

function switchTab(cat, el) {
  currentCat = cat;
  currentBrand = 'all';
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  el.classList.add('active');
  renderAll();
}

// ── برند بار ────────────────────────────────────────────
function renderBrandBar() {
  const filtered = DEVICES.filter(d => d.cat === currentCat);
  const brands = ['all', ...new Set(filtered.map(d => d.brand))];
  const bar = document.getElementById('brandBar');
  bar.innerHTML = '';
  brands.forEach(b => {
    const el = document.createElement('button');
    el.className = 'brand-pill' + (b === currentBrand ? ' active' : '');
    el.textContent = b === 'all' ? 'همه' : b;
    el.onclick = () => { currentBrand = b; renderBrandBar(); renderGrid(); };
    bar.appendChild(el);
  });
}

// ── گرید دستگاه‌ها ──────────────────────────────────────
function renderGrid() {
  const grid = document.getElementById('deviceGrid');
  grid.innerHTML = '';
  const list = DEVICES.filter(d =>
    d.cat === currentCat &&
    (currentBrand === 'all' || d.brand === currentBrand)
  );

  list.forEach(dev => {
    const card = document.createElement('div');
    card.className = 'dev-card';
    card.innerHTML = `
      <div class="dev-card-head">
        <div class="dev-cat-icon">${catIcon(dev.cat, dev.catLabel)}</div>
        <div class="dev-name">${dev.name}</div>
        <div class="dev-brand">${dev.brand}</div>
      </div>`;
    card.onclick = () => openPanel(dev.id);
    grid.appendChild(card);
  });

  const add = document.createElement('div');
  add.className = 'add-card';
  add.innerHTML = `<div class="add-icon">➕</div><div class="add-label">دستگاه جدید</div>`;
  add.onclick = () => openAddModal();
  grid.appendChild(add);
}

// ── باز کردن ریموت ──────────────────────────────────────
function openPanel(devId) {
  const dev = DEVICES.find(d => d.id === devId);
  if (!dev) return;
  openDeviceId = devId;
  learnModeOn = false;
  document.getElementById('rpLearnBtn').classList.remove('on');
  document.getElementById('rpTitle').textContent = dev.name + ' — ' + dev.brand;
  renderPanelBtns(dev);
  document.getElementById('remotePanel').classList.add('open');
  document.getElementById('panelBg').classList.add('show');
}

function closePanel() {
  document.getElementById('remotePanel').classList.remove('open');
  document.getElementById('panelBg').classList.remove('show');
  openDeviceId = null;
  learnModeOn = false;
  document.getElementById('rpLearnBtn').classList.remove('on');
}

function renderPanelBtns(dev) {
  const grid = document.getElementById('rpBtns');
  grid.innerHTML = '';
  dev.btns.forEach(btn => {
    const b = document.createElement('button');
    b.className = 'ir-btn' + (btn.learned ? ' learned' : '');
    b.setAttribute('data-btn', btn.name);
    b.innerHTML = `<span class="bi">${btn.icon}</span><span>${btn.name}</span>`;
    b.onclick = () => {
      if (learnModeOn) {
        startLearnBtn(dev.id, btn.name, b);
      } else {
        sendIR(dev.id, btn.name, b);
      }
    };
    grid.appendChild(b);
  });
}

// ── حالت یادگیری ────────────────────────────────────────
function toggleLearnMode() {
  learnModeOn = !learnModeOn;
  const btn = document.getElementById('rpLearnBtn');
  btn.classList.toggle('on', learnModeOn);
  btn.textContent = learnModeOn ? '✕ لغو یادگیری' : '📡 یادگیری';
  if (learnModeOn) {
    showToast('روی دکمه موردنظر کلیک کن', false);
  } else {
    showToast('حالت یادگیری خاموش شد', false);
  }
}

async function startLearnBtn(devId, btnName, btnEl) {
  pendingLearnBtn = {devId, btnName, el: btnEl};
  btnEl.classList.add('learn-target');
  document.getElementById('learnBtnName').textContent = btnName;
  document.getElementById('learnCountdown').textContent = '15';
  document.getElementById('learnOverlay').classList.add('show');

  await fetch(ESP32_URL + `/learn/start?device=${encodeURIComponent(devId)}&btn=${encodeURIComponent(btnName)}`);
  startLearnPolling(15);
}

function startLearnPolling(sec) {
  let remaining = sec;
  clearInterval(learnCdInterval);
  clearInterval(learnPolling);

  learnCdInterval = setInterval(() => {
    remaining--;
    document.getElementById('learnCountdown').textContent = remaining;
    if (remaining <= 0) clearInterval(learnCdInterval);
  }, 1000);

  learnPolling = setInterval(async () => {
    try {
      const r = await fetch(ESP32_URL + '/learn/status');
      const j = await r.json();
      if (j.status === 'done') {
        clearInterval(learnPolling);
        clearInterval(learnCdInterval);
        onLearnSuccess();
      } else if (j.status === 'timeout' || j.status === 'idle') {
        clearInterval(learnPolling);
        clearInterval(learnCdInterval);
        onLearnTimeout();
      }
    } catch(e) {}
  }, 500);
}

function onLearnSuccess() {
  document.getElementById('learnOverlay').classList.remove('show');
  if (pendingLearnBtn) {
    pendingLearnBtn.el.classList.remove('learn-target');
    pendingLearnBtn.el.classList.add('learned');
    showToast('✅ کد ذخیره شد!', false);
    pendingLearnBtn = null;
  }
  if (learnQueue.length > 0) {
    learnQueueIdx++;
    setTimeout(() => processLearnQueue(), 800);
  } else {
    learnModeOn = false;
    document.getElementById('rpLearnBtn').classList.remove('on');
    document.getElementById('rpLearnBtn').textContent = '📡 یادگیری';
  }
  loadDevices();
}

function onLearnTimeout() {
  document.getElementById('learnOverlay').classList.remove('show');
  if (pendingLearnBtn) {
    pendingLearnBtn.el.classList.remove('learn-target');
    pendingLearnBtn = null;
  }
  if (learnQueue.length > 0) {
    if (confirm('سیگنالی دریافت نشد. این دکمه رو رد کنم و بریم سراغ بعدی؟')) {
      learnQueueIdx++;
      setTimeout(() => processLearnQueue(), 400);
    } else {
      learnQueue = [];
      showToast('یادگیری متوقف شد', true);
    }
  } else {
    showToast('⏰ سیگنالی دریافت نشد', true);
  }
}

async function cancelLearn() {
  clearInterval(learnPolling);
  clearInterval(learnCdInterval);
  await fetch(ESP32_URL + '/learn/cancel');
  document.getElementById('learnOverlay').classList.remove('show');
  if (pendingLearnBtn) {
    pendingLearnBtn.el.classList.remove('learn-target');
    pendingLearnBtn = null;
  }
  learnQueue = [];
  showToast('یادگیری لغو شد', true);
}

// ── ارسال IR ─────────────────────────────────────────────
async function sendIR(devId, btnName, el) {
  el.classList.add('sent');
  try {
    const r = await fetch(ESP32_URL + `/ir?device=${encodeURIComponent(devId)}&btn=${encodeURIComponent(btnName)}`);
    const j = await r.json();
    showToast('📡 ' + btnName, false);
  } catch(e) {
    showToast('⚠️ خطا در اتصال به ESP32', true);
  }
  setTimeout(() => el.classList.remove('sent'), 800);
}

// ── Add device modal ─────────────────────────────────────
function openAddModal() {
  selectedAddBtns.clear();
  renderBtnPicker();
  document.getElementById('newDevName').value = '';
  document.getElementById('newDevBrand').value = '';
  document.getElementById('addModal').classList.add('show');
}

function closeAddModal() {
  document.getElementById('addModal').classList.remove('show');
}

function renderBtnPicker() {
  const grid = document.getElementById('btnPickerGrid');
  grid.innerHTML = '';
  PRESET_BTNS.forEach(b => {
    const el = document.createElement('div');
    el.className = 'btn-add-btn' + (selectedAddBtns.has(b.name) ? ' sel' : '');
    el.innerHTML = b.icon + ' ' + b.name;
    el.onclick = () => {
      selectedAddBtns.has(b.name) ? selectedAddBtns.delete(b.name) : selectedAddBtns.add(b.name);
      renderBtnPicker();
    };
    grid.appendChild(el);
  });
}

function startAddDevice() {
  const name  = document.getElementById('newDevName').value.trim();
  const brand = document.getElementById('newDevBrand').value.trim();
  const cat   = document.getElementById('newDevCat').value;
  if (!name || !brand) { showToast('نام و برند رو پر کن', true); return; }
  if (selectedAddBtns.size === 0) { showToast('حداقل یک دکمه انتخاب کن', true); return; }

  const id = 'custom_' + Date.now();
  addDeviceData = { id, name, brand, cat };

  learnQueue = Array.from(selectedAddBtns).map(bName => {
    const preset = PRESET_BTNS.find(p => p.name === bName);
    return { devId: id, btnName: bName, icon: preset ? preset.icon : '•' };
  });
  learnQueueIdx = 0;

  closeAddModal();

  const catMap = {tv:'تلویزیون', ac:'کولر گازی', other:'سایر'};
  DEVICES.push({
    id, name, brand, cat, catLabel: catMap[cat],
    btns: learnQueue.map(l => ({name:l.btnName, icon:l.icon, learned:false}))
  });
  renderAll();
  openPanel(id);

  setTimeout(() => processLearnQueue(), 600);
}

async function processLearnQueue() {
  if (learnQueueIdx >= learnQueue.length) {
    showToast('🎉 همه دکمه‌ها یاد گرفته شد!', false);
    learnQueue = [];
    return;
  }
  const item = learnQueue[learnQueueIdx];
  const btnEls = document.querySelectorAll('#rpBtns .ir-btn');
  let btnEl = null;
  btnEls.forEach(el => { if(el.getAttribute('data-btn') === item.btnName) btnEl = el; });

  pendingLearnBtn = {devId: item.devId, btnName: item.btnName, el: btnEl};
  if (btnEl) btnEl.classList.add('learn-target');
  document.getElementById('learnBtnName').textContent = item.btnName;
  document.getElementById('learnCountdown').textContent = '15';
  document.getElementById('learnOverlay').classList.add('show');

  await fetch(ESP32_URL + `/learn/start?device=${encodeURIComponent(item.devId)}&btn=${encodeURIComponent(item.btnName)}`);
  startLearnPolling(15);
}

// ── Toast ────────────────────────────────────────────────
function showToast(msg, err) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'toast' + (err ? ' err' : '') + ' show';
  setTimeout(() => t.classList.remove('show'), 2200);
}

// ── Init ─────────────────────────────────────────────────
loadDevices();
