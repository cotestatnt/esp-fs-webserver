// db.js
const mysql = require('mysql2');
require('dotenv').config();

// Creiamo il pool di connessioni usando le variabili d'ambiente
const pool = mysql.createPool({
  host: process.env.DB_HOST,
  port: process.env.DB_PORT,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_NAME,
  waitForConnections: true,
  connectionLimit: 10,
  queueLimit: 0
});

// Esportiamo la versione basata su Promise per usare async/await
module.exports = pool.promise();