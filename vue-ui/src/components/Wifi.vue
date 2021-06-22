<template>
  <div class="container" id="wifi-box">
    <BModal ref="BModal" :message="modalMessage" :title="modalTitle" :buttonType="modalButton" @Connect="doConnection" />
    <h1>esp-fs-webserver</h1>
    <br>
    <div style="margin: auto auto;">
      <button type="button" class="btn btn-primary btn-sm" @click="startScan" >Scan WiFi</button>
    </div>
      <div v-if="scanning"><br>
      <h3>WiFi networks <span class="blink-text" v-if="ssidList.length === 0"> ...scanning</span></h3>
      <div id="wifi-list" class="border rounded-2">
          <table class="table">
              <tbody>
                  <tr class="row">
                      <td class="col-2"><b>Select</b></td>
                      <td class="col-5"><b>SSID Name</b></td>
                      <td class="col-2"><b>RSSI level</b></td>
                      <td class="col-2"><b>Security</b></td>
                  </tr>
                  <tr v-for="item in ssidList" :key="item.ssid" class="row">
                      <td class="col-2">
                          <div class="form-check">
                          <input
                              type="checkbox"
                              class="form-check-input"
                              :id="item.ssid"
                              v-model="item.status"
                              @change="updateSelectedNet(item)"
                          />
                          </div>
                      </td>
                      <td class="col-5">{{ item.ssid }}</td>
                      <td class="col-2">{{ item.strength }}</td>
                      <td class="col-2">
                          <div v-if=item.security>
                              <svg height="16pt" viewBox="0 0 512 512"><path d="m336 512h-288c-26.453125 0-48-21.523438-48-48v-224c0-26.476562 21.546875-48 48-48h288c26.453125 0 48 21.523438 48 48v224c0 26.476562-21.546875 48-48 48zm-288-288c-8.8125 0-16 7.167969-16 16v224c0 8.832031 7.1875 16 16 16h288c8.8125 0 16-7.167969 16-16v-224c0-8.832031-7.1875-16-16-16zm0 0"/><path d="m304 224c-8.832031 0-16-7.167969-16-16v-80c0-52.929688-43.070312-96-96-96s-96 43.070312-96 96v80c0 8.832031-7.167969 16-16 16s-16-7.167969-16-16v-80c0-70.59375 57.40625-128 128-128s128 57.40625 128 128v80c0 8.832031-7.167969 16-16 16zm0 0"/></svg>
                          </div>
                          <div v-else>
                              <svg height="16pt" viewBox="0 0 512 512"><path d="m336 512h-288c-26.453125 0-48-21.523438-48-48v-224c0-26.476562 21.546875-48 48-48h288c26.453125 0 48 21.523438 48 48v224c0 26.476562-21.546875 48-48 48zm-288-288c-8.8125 0-16 7.167969-16 16v224c0 8.832031 7.1875 16 16 16h288c8.8125 0 16-7.167969 16-16v-224c0-8.832031-7.1875-16-16-16zm0 0"/><path d="m80 224c-8.832031 0-16-7.167969-16-16v-80c0-70.59375 57.40625-128 128-128s128 57.40625 128 128c0 8.832031-7.167969 16-16 16s-16-7.167969-16-16c0-52.929688-43.070312-96-96-96s-96 43.070312-96 96v80c0 8.832031-7.167969 16-16 16zm0 0"/></svg>
                          </div>
                      </td>
                  </tr>
              </tbody>
          </table>
      </div>
    </div>
    <br>
    <div class="input-group mb-3 sm float-right" style="widht:60%">
        <div class="input-group-prepend"> <span class="input-group-text">WiFi SSID</span> </div>
        <input type="text" class="form-control" v-model=ssid>
    </div>

    <div class="input-group mb-3 sm">
        <div class="input-group-prepend"> <span class="input-group-text">WiFi password</span> </div>
        <input type="password" name="pwd" id="input-pwd" v-model=password class="form-control validate" required>
        <span toggle="#input-pwd" class="fa fa-fw fa-eye field-icon toggle-password"></span>
    </div>
    <button class="btn btn-outline-primary"
      v-if="ssidList.length != 0"
      type="button"
      @click="doConnection">Connect to {{ssid}}
    </button>

    <h3 v-if="numOptions > 0"><br><br><br>Additional parameters
      <span class="blink-text" v-if="numOptions === 0 && optionList.constructor === Object">  ...loading</span>
    </h3>
    <div class="form-group">
        <div v-for="(item, key, index) in optionList" :key="index" class="input-group mb-3 sm">
            <div class="input-group-prepend">
                <span class="input-group-text" v-if="typeof item !== 'boolean'" >
                  {{key}}
                </span>
            </div>
            <input v-if="typeof item === 'string'"
                type="text" class="form-control"
                v-model="optionList[key]"
            />
            <input v-if="typeof item === 'number'"
                type="number" class="form-control"
                v-model="optionList[key]"
            />
            <div v-if="typeof item === 'boolean'" >
                <input type="checkbox" style="margin-right: 15px;"
                    class="form-check-input"
                    v-model="optionList[key]"
                />
                {{ key }}
            </div>
        </div>
    </div>

    <button type="button" class="btn btn-primary sm" @click="saveConfig" >Save configuration</button>
  </div>
</template>


<script>
import axios from 'axios';
import BModal from "../components/BModal";

export default {
  name: 'ScanWifi',

  components: {
    BModal
  },

  data() {
    return {
        modalButton: '',
        modalTitle: 'info',
        modalMessage: 'test message',
        scanning: false,
        ssid: '',
        password: '',
        espHostname: '',
        currentIndex: -1,
        ssidList: [],
        selectedItem: {},
        optionList: {},
        numOptions: 0,
    };
  },

  mounted() {
    this.getOptionList();
  },

  methods: {

    async doConnection() {
      if(!this.password || !this.ssid) {
        this.modalTitle = 'WiFi credentials missing' ;
        this.$refs.BModal.toggleModal();
      }

      let uri = '/connect?ssid=' + this.ssid + '&password=' + this.password;
      axios
        .get(uri)
        .then(response => {
            console.log(response);
            setTimeout( () => {
              window.location.href = 'http://' + this.espHostame + '/setup';
            }, 3000);
        });
    },

    async startScan() {
      this.scanning = true;
      await this.getNetworkList();
    },

    updateSelectedNet(item) {
      this.selectedItem = item;
      this.ssid = this.selectedItem.ssid;
    },

    saveConfig() {
        let conn = {};
        conn.ssid = this.ssid;
        conn.password = this.password;

        var obj = Object.assign(conn, this.optionList);
        var myblob = new Blob([JSON.stringify(obj, null, 2)], {
          type: 'application/json'
        });
        var formData = new FormData();
        formData.append("data", myblob, '/config.json');

        const headers = {
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Max-Age': '600',
            'Access-Control-Allow-Methods': 'PUT,POST,GET,OPTIONS',
            'Access-Control-Allow-Headers': '*'
        };

        axios
        .post('/edit', formData, { headers })
        .then(response => {
            this.modalTitle = 'WiFi credentials and application options' ;
            this.modalMessage = 'File <b>config.json</b> saved successfully!<br><br>' +
                                'If not already done, you have to authorize the application by pressing the lower button and following provided instructions';
            this.modalButton =  'Connect';
            this.$refs.BModal.toggleModal();
            console.log(response);
        });
    },

    async getNetworkList() {
      axios
        .get('/scan')
        .then(response => {
            this.ssidList = response.data;
            this.ssidList.forEach(el => {
            if(el.selected === true)
                this.ssid = el.ssid
                this.password = el.password
            })
        });
    },

    async getOptionList() {
      axios
        .get('/config.json')
        .then(response => {
          const raw = response.data
          this.espHostame = raw['hostname']
          this.ssid = raw['ssid']
          this.password = raw['password']
          const notallowed = ['ssid', 'password'];
          const filtered = Object.keys(raw)
            .filter(key => !notallowed.includes(key))
            .reduce((obj, key) => {
              obj[key] = raw[key];
              return obj;
            }, {});
          this.optionList = filtered;
          this.numOptions = Object.keys(this.optionList).length;
        })
        .catch((error) => {
          console.log('config.json not found' + error);
          this.numOptions = -1;
        });
    },

  },
}

</script>

<style scoped>
h3{
    text-align: left;
    font-size: 1.25rem;
}
#wifi-list{
    padding: 10px;
}

.blink-text {
  animation: blinker 1s linear infinite;
}

@keyframes blinker {
  50% {
    opacity: 0;
  }
}

.btn {
  float: right;
  margin-bottom: 20px;
}
</style>
