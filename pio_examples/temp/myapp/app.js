// app.js
require('dotenv').config();
const express = require('express');
const db = require('./db');
const productsRouter = require('./routes/products');
const customersRouter = require('./routes/customers');
const ordersRouter = require('./routes/orders');

const app = express();
app.use(express.json());
app.use(express.static('public'));

const PORT = process.env.PORT || 3000;

// Rotta di esempio per testare la connessione
app.get('/test-db', async (req, res) => {
  try {
    const [rows] = await db.query('SELECT NOW() as currentTime');
    res.json({
      message: 'Connesso a MariaDB!',
      data: rows[0]
    });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Errore di connessione al database' });
  }
});

app.use('/products', productsRouter);
app.use('/customers', customersRouter);
app.use('/orders', ordersRouter);

app.listen(PORT, () => {
  console.log(`Server in esecuzione sulla porta ${PORT}`);
});