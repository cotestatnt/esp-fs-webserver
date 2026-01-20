const express = require('express');
const db = require('../db');

const router = express.Router();

router.get('/', async (req, res) => {
  try {
    const [rows] = await db.query(
      'SELECT id, sku, name, price_cents, stock, active, created_at FROM products ORDER BY id DESC'
    );
    res.json(rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nel recupero dei prodotti' });
  }
});

router.get('/:id', async (req, res) => {
  try {
    const [rows] = await db.query(
      'SELECT id, sku, name, price_cents, stock, active, created_at FROM products WHERE id = ?',
      [req.params.id]
    );
    if (rows.length === 0) {
      return res.status(404).json({ error: 'Prodotto non trovato' });
    }
    res.json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nel recupero del prodotto' });
  }
});

router.post('/', async (req, res) => {
  try {
    const { sku, name, price_cents, stock = 0, active = 1 } = req.body;

    if (!sku || !name || price_cents === undefined) {
      return res.status(400).json({ error: 'Campi obbligatori: sku, name, price_cents' });
    }

    const [result] = await db.query(
      'INSERT INTO products (sku, name, price_cents, stock, active) VALUES (?, ?, ?, ?, ?)',
      [sku, name, price_cents, stock, active]
    );

    const [rows] = await db.query(
      'SELECT id, sku, name, price_cents, stock, active, created_at FROM products WHERE id = ?',
      [result.insertId]
    );

    res.status(201).json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nella creazione del prodotto' });
  }
});

router.put('/:id', async (req, res) => {
  try {
    const { sku, name, price_cents, stock, active } = req.body;

    const fields = [];
    const values = [];

    if (sku !== undefined) {
      fields.push('sku = ?');
      values.push(sku);
    }
    if (name !== undefined) {
      fields.push('name = ?');
      values.push(name);
    }
    if (price_cents !== undefined) {
      fields.push('price_cents = ?');
      values.push(price_cents);
    }
    if (stock !== undefined) {
      fields.push('stock = ?');
      values.push(stock);
    }
    if (active !== undefined) {
      fields.push('active = ?');
      values.push(active);
    }

    if (fields.length === 0) {
      return res.status(400).json({ error: 'Nessun campo da aggiornare' });
    }

    values.push(req.params.id);

    const [result] = await db.query(
      `UPDATE products SET ${fields.join(', ')} WHERE id = ?`,
      values
    );

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Prodotto non trovato' });
    }

    const [rows] = await db.query(
      'SELECT id, sku, name, price_cents, stock, active, created_at FROM products WHERE id = ?',
      [req.params.id]
    );

    res.json(rows[0]);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nell’aggiornamento del prodotto' });
  }
});

router.delete('/:id', async (req, res) => {
  try {
    const [result] = await db.query('DELETE FROM products WHERE id = ?', [req.params.id]);

    if (result.affectedRows === 0) {
      return res.status(404).json({ error: 'Prodotto non trovato' });
    }

    res.status(204).send();
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore nella cancellazione del prodotto' });
  }
});

module.exports = router;
