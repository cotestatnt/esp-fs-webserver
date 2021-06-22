<template>
  <div id="app">
    <div class="container" style="max-width: 600px;">
      <div class="alert alert-warning alert-dismissible fade show" role="alert" v-if="chrome">
        Due to a bug with ESP32@Chrome, this webpage could not be loaded properly.<br>
        If you encounter problems, try a different browser (for example Firefox)
        <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close" @click="dismiss"></button>
      </div>
      <AppLogo class="app-logo" />
      <WifiScan />
     </div>
  </div>
</template>

<script>
import WifiScan from './components/Wifi.vue'
import AppLogo  from './assets/svg-logo.vue'

export default {
  name: 'App',
  components: {
    WifiScan,
    AppLogo,
  },
  data() {
    return {
        chrome: false,
    };
  },

  methods: {
    dismiss() {
      this.chrome = false;
    },

    checkBrowser(){
      let chromeAgent = navigator.userAgent.indexOf("Chrome") > -1
      let edgeAgent = navigator.userAgent.indexOf("Edge") > -1
      if( chromeAgent || edgeAgent)
        this.chrome = true;
    }
  },

  mounted() {
    // this.checkBrowser()
  }
}
</script>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  color: #2c3e50;
  margin-top: 30px;
  line-height: 1.2;
}
.app-logo{
  max-height: 128px;
}
</style>
