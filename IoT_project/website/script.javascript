function eventLabel(type) {
  if (type === 1) return ['RFID', 'badge-rfid'];
  if (type === 2) return ['IR_CHANGE', 'badge-ir'];
  if (type === 3) return ['ENTRY_COUNTED', 'badge-entry'];
  return ['UNKNOWN', 'badge-ir'];
}

function formatTime(ts) {
  const d = ts ? new Date(ts) : new Date();
  return d.toLocaleTimeString();
}

async function refreshState() {
  const res = await fetch('/api/state');
  if (!res.ok) return;

  const state = await res.json();
  document.getElementById('total-spaces').textContent = state.total_spaces;
  document.getElementById('empty-spaces').textContent = state.empty_spaces;
  document.getElementById('occupied-spaces').textContent = state.occupied_spaces;
  document.getElementById('ir-state').textContent = state.ir_occupied ? 'BLOCKED' : 'CLEAR';
  document.getElementById('last-uid').textContent = state.last_uid || 'none';
}

async function refreshEvents() {
  const res = await fetch('/api/events?limit=25');
  if (!res.ok) return;

  const events = await res.json();
  const tbody = document.getElementById('events-body');
  tbody.innerHTML = '';

  for (const evt of events) {
    const [label, cls] = eventLabel(evt.event_type);
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td>${formatTime(evt.timestamp_ms)}</td>
      <td><span class="badge ${cls}">${label}</span></td>
      <td>${evt.uid || '-'}</td>
      <td>${evt.empty_spaces}</td>
      <td>${evt.occupied_spaces}</td>
    `;
    tbody.appendChild(tr);
  }
}

async function refreshAll() {
  try {
    await Promise.all([refreshState(), refreshEvents()]);
  } catch (err) {
    console.error(err);
  }
}

setInterval(refreshAll, 2000);
refreshAll();
