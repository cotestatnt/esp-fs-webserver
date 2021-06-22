import Vue from 'vue'
import App from './App.vue'

// import 'bootstrap';
import '@/scss/custom.scss';
Vue.config.productionTip = false
Vue.config.devtools = false

new Vue({
  render: h => h(App),
}).$mount('#app')
