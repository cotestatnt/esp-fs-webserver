const productsBody = document.getElementById('productsBody');
const totalProducts = document.getElementById('totalProducts');
const totalStock = document.getElementById('totalStock');
const totalValue = document.getElementById('totalValue');
const searchInput = document.getElementById('searchInput');
const customersBody = document.getElementById('customersBody');
const ordersBody = document.getElementById('ordersBody');

const modal = document.getElementById('modal');
const modalTitle = document.getElementById('modalTitle');
const productForm = document.getElementById('productForm');
const productId = document.getElementById('productId');
const skuInput = document.getElementById('sku');
const nameInput = document.getElementById('name');
const priceInput = document.getElementById('price_cents');
const stockInput = document.getElementById('stock');
const activeInput = document.getElementById('active');

const openCreate = document.getElementById('openCreate');
const closeModal = document.getElementById('closeModal');
const cancelModal = document.getElementById('cancelModal');

const customerModal = document.getElementById('customerModal');
const customerModalTitle = document.getElementById('customerModalTitle');
const customerForm = document.getElementById('customerForm');
const customerId = document.getElementById('customerId');
const customerEmail = document.getElementById('customerEmail');
const customerName = document.getElementById('customerName');
const customerPhone = document.getElementById('customerPhone');
const openCustomerCreate = document.getElementById('openCustomerCreate');
const closeCustomerModal = document.getElementById('closeCustomerModal');
const cancelCustomerModal = document.getElementById('cancelCustomerModal');

const orderModal = document.getElementById('orderModal');
const orderModalTitle = document.getElementById('orderModalTitle');
const orderForm = document.getElementById('orderForm');
const orderId = document.getElementById('orderId');
const orderCustomer = document.getElementById('orderCustomer');
const orderStatus = document.getElementById('orderStatus');
const orderTotal = document.getElementById('orderTotal');
const openOrderCreate = document.getElementById('openOrderCreate');
const closeOrderModal = document.getElementById('closeOrderModal');
const cancelOrderModal = document.getElementById('cancelOrderModal');

const navItems = document.querySelectorAll('.nav-item');
const sections = document.querySelectorAll('.section');

let products = [];
let customers = [];
let orders = [];

const euro = (cents) =>
  new Intl.NumberFormat('it-IT', {
    style: 'currency',
    currency: 'EUR'
  }).format((cents || 0) / 100);

const openModal = (product) => {
  modal.classList.remove('hidden');
  if (product) {
    modalTitle.textContent = 'Modifica prodotto';
    productId.value = product.id;
    skuInput.value = product.sku;
    nameInput.value = product.name;
    priceInput.value = product.price_cents;
    stockInput.value = product.stock;
    activeInput.checked = Boolean(product.active);
  } else {
    modalTitle.textContent = 'Nuovo prodotto';
    productId.value = '';
    productForm.reset();
    activeInput.checked = true;
  }
};

const closeModalFn = () => {
  modal.classList.add('hidden');
};

const openCustomerModal = (customer) => {
  customerModal.classList.remove('hidden');
  if (customer) {
    customerModalTitle.textContent = 'Modifica cliente';
    customerId.value = customer.id;
    customerEmail.value = customer.email;
    customerName.value = customer.name;
    customerPhone.value = customer.phone || '';
  } else {
    customerModalTitle.textContent = 'Nuovo cliente';
    customerId.value = '';
    customerForm.reset();
  }
};

const closeCustomerModalFn = () => {
  customerModal.classList.add('hidden');
};

const openOrderModal = (order) => {
  orderModal.classList.remove('hidden');
  if (order) {
    orderModalTitle.textContent = 'Modifica ordine';
    orderId.value = order.id;
    orderCustomer.value = order.customer_id;
    orderStatus.value = order.status;
    orderTotal.value = order.total_cents;
  } else {
    orderModalTitle.textContent = 'Nuovo ordine';
    orderId.value = '';
    orderForm.reset();
    orderStatus.value = 'NEW';
    orderTotal.value = 0;
  }
};

const closeOrderModalFn = () => {
  orderModal.classList.add('hidden');
};

const renderStats = (items) => {
  totalProducts.textContent = items.length;
  const stock = items.reduce((sum, item) => sum + (item.stock || 0), 0);
  const value = items.reduce(
    (sum, item) => sum + (item.price_cents || 0) * (item.stock || 0),
    0
  );
  totalStock.textContent = stock;
  totalValue.textContent = euro(value);
};

const renderTable = (items) => {
  productsBody.innerHTML = '';
  items.forEach((product) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td>${product.sku}</td>
      <td>${product.name}</td>
      <td>${euro(product.price_cents)}</td>
      <td>${product.stock}</td>
      <td>
        <span class="badge ${product.active ? 'active' : 'inactive'}">
          ${product.active ? 'Attivo' : 'Disattivo'}
        </span>
      </td>
      <td class="actions">
        <button class="btn" data-edit="${product.id}">Modifica</button>
        <button class="btn" data-delete="${product.id}">Elimina</button>
      </td>
    `;
    productsBody.appendChild(tr);
  });
};

const renderCustomers = (items) => {
  customersBody.innerHTML = '';
  items.forEach((customer) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td>${customer.email}</td>
      <td>${customer.name}</td>
      <td>${customer.phone || '-'}</td>
      <td>${new Date(customer.created_at).toLocaleDateString('it-IT')}</td>
      <td class="actions">
        <button class="btn" data-customer-edit="${customer.id}">Modifica</button>
        <button class="btn" data-customer-delete="${customer.id}">Elimina</button>
      </td>
    `;
    customersBody.appendChild(tr);
  });
};

const renderOrders = (items) => {
  ordersBody.innerHTML = '';
  items.forEach((order) => {
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td>#${order.id}</td>
      <td>${order.customer_name}</td>
      <td>${order.status}</td>
      <td>${euro(order.total_cents)}</td>
      <td>${new Date(order.created_at).toLocaleDateString('it-IT')}</td>
      <td class="actions">
        <button class="btn" data-order-edit="${order.id}">Modifica</button>
        <button class="btn" data-order-delete="${order.id}">Elimina</button>
      </td>
    `;
    ordersBody.appendChild(tr);
  });
};

const loadProducts = async () => {
  const response = await fetch('/products');
  products = await response.json();
  renderStats(products);
  renderTable(products);
};

const loadCustomers = async () => {
  const response = await fetch('/customers');
  customers = await response.json();
  renderCustomers(customers);
  renderCustomerOptions();
};

const loadOrders = async () => {
  const response = await fetch('/orders');
  orders = await response.json();
  renderOrders(orders);
};

const renderCustomerOptions = () => {
  orderCustomer.innerHTML = '';
  customers.forEach((customer) => {
    const option = document.createElement('option');
    option.value = customer.id;
    option.textContent = `${customer.name} (${customer.email})`;
    orderCustomer.appendChild(option);
  });
};

const filterProducts = () => {
  const value = searchInput.value.toLowerCase();
  const filtered = products.filter(
    (p) => p.name.toLowerCase().includes(value) || p.sku.toLowerCase().includes(value)
  );
  renderStats(filtered);
  renderTable(filtered);
};

const saveProduct = async (event) => {
  event.preventDefault();
  const payload = {
    sku: skuInput.value.trim(),
    name: nameInput.value.trim(),
    price_cents: Number(priceInput.value),
    stock: Number(stockInput.value || 0),
    active: activeInput.checked ? 1 : 0
  };

  let response;
  if (productId.value) {
    response = await fetch(`/products/${productId.value}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
  } else {
    response = await fetch('/products', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
  }

  if (!response.ok) {
    alert('Errore nel salvataggio del prodotto');
    return;
  }

  closeModalFn();
  await loadProducts();
};

const deleteProduct = async (id) => {
  if (!confirm('Eliminare il prodotto?')) return;
  const response = await fetch(`/products/${id}`, { method: 'DELETE' });
  if (!response.ok) {
    alert('Errore nella cancellazione del prodotto');
    return;
  }
  await loadProducts();
};

const saveCustomer = async (event) => {
  event.preventDefault();
  const payload = {
    email: customerEmail.value.trim(),
    name: customerName.value.trim(),
    phone: customerPhone.value.trim() || null
  };

  let response;
  if (customerId.value) {
    response = await fetch(`/customers/${customerId.value}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
  } else {
    response = await fetch('/customers', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
  }

  if (!response.ok) {
    alert('Errore nel salvataggio del cliente');
    return;
  }

  closeCustomerModalFn();
  await loadCustomers();
};

const deleteCustomer = async (id) => {
  if (!confirm('Eliminare il cliente?')) return;
  const response = await fetch(`/customers/${id}`, { method: 'DELETE' });
  if (!response.ok) {
    alert('Errore nella cancellazione del cliente');
    return;
  }
  await loadCustomers();
};

const saveOrder = async (event) => {
  event.preventDefault();
  const payload = {
    customer_id: Number(orderCustomer.value),
    status: orderStatus.value,
    total_cents: Number(orderTotal.value || 0)
  };

  let response;
  if (orderId.value) {
    response = await fetch(`/orders/${orderId.value}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
  } else {
    response = await fetch('/orders', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
  }

  if (!response.ok) {
    alert('Errore nel salvataggio dell’ordine');
    return;
  }

  closeOrderModalFn();
  await loadOrders();
};

const deleteOrder = async (id) => {
  if (!confirm('Eliminare l’ordine?')) return;
  const response = await fetch(`/orders/${id}`, { method: 'DELETE' });
  if (!response.ok) {
    alert('Errore nella cancellazione dell’ordine');
    return;
  }
  await loadOrders();
};

productsBody.addEventListener('click', (event) => {
  const editId = event.target.dataset.edit;
  const deleteId = event.target.dataset.delete;
  if (editId) {
    const product = products.find((p) => String(p.id) === editId);
    openModal(product);
  }
  if (deleteId) {
    deleteProduct(deleteId);
  }
});

customersBody.addEventListener('click', (event) => {
  const editId = event.target.dataset.customerEdit;
  const deleteId = event.target.dataset.customerDelete;
  if (editId) {
    const customer = customers.find((c) => String(c.id) === editId);
    openCustomerModal(customer);
  }
  if (deleteId) {
    deleteCustomer(deleteId);
  }
});

ordersBody.addEventListener('click', (event) => {
  const editId = event.target.dataset.orderEdit;
  const deleteId = event.target.dataset.orderDelete;
  if (editId) {
    const order = orders.find((o) => String(o.id) === editId);
    openOrderModal(order);
  }
  if (deleteId) {
    deleteOrder(deleteId);
  }
});

navItems.forEach((item) => {
  item.addEventListener('click', (event) => {
    event.preventDefault();
    const target = item.dataset.section;
    navItems.forEach((nav) => nav.classList.remove('active'));
    item.classList.add('active');
    sections.forEach((section) => {
      section.classList.toggle('hidden', section.dataset.section !== target);
    });
  });
});

openCreate.addEventListener('click', () => openModal());
closeModal.addEventListener('click', closeModalFn);
cancelModal.addEventListener('click', closeModalFn);
productForm.addEventListener('submit', saveProduct);
searchInput.addEventListener('input', filterProducts);

openCustomerCreate.addEventListener('click', () => openCustomerModal());
closeCustomerModal.addEventListener('click', closeCustomerModalFn);
cancelCustomerModal.addEventListener('click', closeCustomerModalFn);
customerForm.addEventListener('submit', saveCustomer);

openOrderCreate.addEventListener('click', () => openOrderModal());
closeOrderModal.addEventListener('click', closeOrderModalFn);
cancelOrderModal.addEventListener('click', closeOrderModalFn);
orderForm.addEventListener('submit', saveOrder);

loadProducts();
loadCustomers();
loadOrders();
