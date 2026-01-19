const express = require('express');
const db = require('../db');

const router = express.Router();

router.get('/', async (req, res) => {
  try {
    const [rows] = await db.query(
      'SELECT id, email, name, phone, created_at FROM customers ORDER BY id DESC'
    );
    res.json(rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nel recupero dei clienti' });
  }
});

router.get('/:id', async (req, res) => {
  try {
    const [rows] = await db.query(
      'SELECT id, email, name, phone, created_at FROM customers WHERE id = ?',
      [req.params.id]
    );
    if (rows.length === 0) {
      return res.status(404).json({ error: 'Cliente non trovato' });
    }
    res.json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nel recupero del cliente' });
  }
});

router.post('/', async (req, res) => {
  try {
    const { email, name, phone = null } = req.body;

    if (!email || !name) {
      return res.status(400).json({ error: 'Campi obbligatori: email, name' });
    }

    const [result] = await db.query(
      'INSERT INTO customers (email, name, phone) VALUES (?, ?, ?)',
      [email, name, phone]
    );

    const [rows] = await db.query(
      'SELECT id, email, name, phone, created_at FROM customers WHERE id = ?',
      [result.insertId]
    );

    res.status(201).json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nella creazione del cliente' });
  }
});

router.put('/:id', async (req, res) => {
  try {
    const { email, name, phone } = req.body;

    const fields = [];
    const values = [];

    if (email !== undefined) {
      fields.push('email = ?');
      values.push(email);
    }
    if (name !== undefined) {
      fields.push('name = ?');
      values.push(name);
    }
    if (phone !== undefined) {
      fields.push('phone = ?');
      values.push(phone);
    }

    if (fields.length === 0) {
      return res.status(400).json({ error: 'Nessun campo da aggiornare' });
    }

    values.push(req.params.id);

    const [result] = await db.query(
      `UPDATE customers SET ${fields.join(', ')} WHERE id = ?`,
      values
    );

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Cliente non trovato' });
    }

    const [rows] = await db.query(
      'SELECT id, email, name, phone, created_at FROM customers WHERE id = ?',
      [req.params.id]
    );

    res.json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nell’aggiornamento del cliente' });
  }
});

router.delete('/:id', async (req, res) => {
  try {
    const [result] = await db.query('DELETE FROM customers WHERE id = ?', [req.params.id]);

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Cliente non trovato' });
    }

    res.status(204).send();
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nella cancellazione del cliente' });
  }
});

module.exports = router;
