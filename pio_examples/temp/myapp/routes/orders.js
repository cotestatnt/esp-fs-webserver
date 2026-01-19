const express = require('express');
const db = require('../db');

const router = express.Router();

router.get('/', async (req, res) => {
  try {
    const [rows] = await db.query(
      `SELECT o.id, o.customer_id, c.name AS customer_name, o.status, o.total_cents, o.created_at
       FROM orders o
       JOIN customers c ON c.id = o.customer_id
       ORDER BY o.id DESC`
    );
    res.json(rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nel recupero degli ordini' });
  }
});

router.get('/:id', async (req, res) => {
  try {
    const [rows] = await db.query(
      `SELECT o.id, o.customer_id, c.name AS customer_name, o.status, o.total_cents, o.created_at
       FROM orders o
       JOIN customers c ON c.id = o.customer_id
       WHERE o.id = ?`,
      [req.params.id]
    );
    if (rows.length === 0) {
      return res.status(404).json({ error: 'Ordine non trovato' });
    }
    res.json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nel recupero dell’ordine' });
  }
});

router.post('/', async (req, res) => {
  try {
    const { customer_id, status = 'NEW', total_cents = 0 } = req.body;

    if (!customer_id) {
      return res.status(400).json({ error: 'Campo obbligatorio: customer_id' });
    }

    const [result] = await db.query(
      'INSERT INTO orders (customer_id, status, total_cents) VALUES (?, ?, ?)',
      [customer_id, status, total_cents]
    );

    const [rows] = await db.query(
      `SELECT o.id, o.customer_id, c.name AS customer_name, o.status, o.total_cents, o.created_at
       FROM orders o
       JOIN customers c ON c.id = o.customer_id
       WHERE o.id = ?`,
      [result.insertId]
    );

    res.status(201).json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nella creazione dell’ordine' });
  }
});

router.put('/:id', async (req, res) => {
  try {
    const { customer_id, status, total_cents } = req.body;

    const fields = [];
    const values = [];

    if (customer_id !== undefined) {
      fields.push('customer_id = ?');
      values.push(customer_id);
    }
    if (status !== undefined) {
      fields.push('status = ?');
      values.push(status);
    }
    if (total_cents !== undefined) {
      fields.push('total_cents = ?');
      values.push(total_cents);
    }

    if (fields.length === 0) {
      return res.status(400).json({ error: 'Nessun campo da aggiornare' });
    }

    values.push(req.params.id);

    const [result] = await db.query(
      `UPDATE orders SET ${fields.join(', ')} WHERE id = ?`,
      values
    );

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Ordine non trovato' });
    }

    const [rows] = await db.query(
      `SELECT o.id, o.customer_id, c.name AS customer_name, o.status, o.total_cents, o.created_at
       FROM orders o
       JOIN customers c ON c.id = o.customer_id
       WHERE o.id = ?`,
      [req.params.id]
    );

    res.json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nell’aggiornamento dell’ordine' });
  }
});

router.delete('/:id', async (req, res) => {
  try {
    const [result] = await db.query('DELETE FROM orders WHERE id = ?', [req.params.id]);

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Ordine non trovato' });
    }

    res.status(204).send();
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nella cancellazione dell’ordine' });
  }
});

module.exports = router;
