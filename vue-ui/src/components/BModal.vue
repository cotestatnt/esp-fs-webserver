<template>
  <div>
    <div ref="modal" class="modal fade" :class="{show, 'd-block': active}"  tabindex="-1" role="dialog" >
      <div class="modal-dialog modal-dialog-centered" role="document">
        <div class="modal-content">
          <div class="modal-header">
            <span v-html="title"></span>
            <button v-if="buttonType != ''" class="btn btn-outline-primary" type="button" data-dismiss="modal" @click="emitButton">{{buttonType}}</button>
            <button class="btn btn-outline-primary" type="button" data-dismiss="modal" @click="toggleModal">Close</button>
          </div>
          <div class="modal-body">
            <p><span v-html="message"></span></p>
          </div>
        </div>
      </div>
    </div>
    <div v-if="active" class="modal-backdrop fade show"></div>
  </div>
</template>

<script>

export default {
  props: ['title', 'message', 'buttonType'],

  data() {
    return {
      active: false,
      show: false,
    };
  },
  methods: {
    emitButton() {
      this.$emit(this.buttonType);
      this.toggleModal();
    },

    toggleModal() {
      const body = document.querySelector("body");
      this.active = !this.active;
      this.active
        ? body.classList.add("modal-open")
        : body.classList.remove("modal-open");
      setTimeout(() => (this.show = !this.show), 10);
    }
  }
};
</script>

