const express = require('express');
const sqlite3 = require('sqlite3').verbose();
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = process.env.PORT || 3000;
const DB_PATH = path.join(__dirname, 'parking.db');

const db = new sqlite3.Database(DB_PATH);

db.serialize(() => {
  db.run(`
    CREATE TABLE IF NOT EXISTS events (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      event_type INTEGER NOT NULL,
      uid TEXT,
      total_spaces INTEGER,
      empty_spaces INTEGER,
      occupied_spaces INTEGER,
      ir_occupied INTEGER,
      timestamp_ms INTEGER,
      created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `);

  db.run(`
    CREATE TABLE IF NOT EXISTS parking_state (
      id INTEGER PRIMARY KEY CHECK(id = 1),
      total_spaces INTEGER NOT NULL,
      empty_spaces INTEGER NOT NULL,
      occupied_spaces INTEGER NOT NULL,
      ir_occupied INTEGER NOT NULL,
      last_uid TEXT,
      updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `);

  db.run(
    `INSERT OR IGNORE INTO parking_state(id, total_spaces, empty_spaces, occupied_spaces, ir_occupied, last_uid)
     VALUES (1, 50, 50, 0, 0, '')`
  );
});

app.use(cors());
app.use(express.json());
app.use(express.static(__dirname));

app.get('/api/state', (req, res) => {
  db.get('SELECT * FROM parking_state WHERE id = 1', (err, row) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }
    res.json(row);
  });
});

app.get('/api/events', (req, res) => {
  const limit = Math.min(parseInt(req.query.limit || '50', 10), 200);
  db.all(
    'SELECT * FROM events ORDER BY id DESC LIMIT ?',
    [limit],
    (err, rows) => {
      if (err) {
        return res.status(500).json({ error: err.message });
      }
      res.json(rows);
    }
  );
});

app.post('/api/events', (req, res) => {
  const {
    event_type,
    uid,
    total_spaces,
    empty_spaces,
    occupied_spaces,
    ir_occupied,
    timestamp_ms,
  } = req.body;

  if (typeof event_type !== 'number') {
    return res.status(400).json({ error: 'event_type is required' });
  }

  const stmt = db.prepare(`
    INSERT INTO events (
      event_type,
      uid,
      total_spaces,
      empty_spaces,
      occupied_spaces,
      ir_occupied,
      timestamp_ms
    ) VALUES (?, ?, ?, ?, ?, ?, ?)
  `);

  stmt.run(
    event_type,
    uid || '',
    total_spaces ?? 0,
    empty_spaces ?? 0,
    occupied_spaces ?? 0,
    ir_occupied ? 1 : 0,
    timestamp_ms ?? Date.now(),
    function onInsert(err) {
      if (err) {
        return res.status(500).json({ error: err.message });
      }

      db.run(
        `UPDATE parking_state
         SET total_spaces = ?, empty_spaces = ?, occupied_spaces = ?, ir_occupied = ?, last_uid = ?, updated_at = CURRENT_TIMESTAMP
         WHERE id = 1`,
        [
          total_spaces ?? 0,
          empty_spaces ?? 0,
          occupied_spaces ?? 0,
          ir_occupied ? 1 : 0,
          uid || '',
        ],
        (stateErr) => {
          if (stateErr) {
            return res.status(500).json({ error: stateErr.message });
          }

          res.json({ ok: true, id: this.lastID });
        }
      );
    }
  );

  stmt.finalize();
});

app.listen(PORT, () => {
  console.log(`Smart parking web server running on http://localhost:${PORT}`);
});
