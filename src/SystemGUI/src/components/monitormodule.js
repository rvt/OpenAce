import { El } from "@frameable/el";
import store from "./store";
import "./moduleconfigs";

class MonitorModule extends El {
  created() {
    this.state = this.$observable({ data: [] });
  }

  mounted() {
    this._running = true;
    this._fetchData();
  }

  unmounted() {
    this._running = false;
    clearTimeout(this.timer);
  }

  _fetchData() {
    store
      .fetch(`/api/${this.selected}.json`)
      .then((data) => {
        this.state.data.length = 0;
        for (var prop in data) {
          this.state.data.push({ name: prop, value: data[prop] });
        }
      })
      .catch((e) => {
        this.state.data.length = 0;
      })
      .finally(() => {
        if (this._running) {
          this.timer = this.timer = setTimeout(() => {
            this._fetchData();
          }, 750);
        }
      });
  }

  _row(html, item) {
    var value;
    if (Array.isArray(item.value)) {
      value = item.value.join(", ");
    } else {
      value = item.value;
    }

    return html`
      <tr>
        <th style="width:33%" scope="row">${item.name}</th>
        <td>${value}</td>
      </tr>
    `;
  }

  _filteredItems() {
    return this.state.data.filter((i) => true);
  }

  render(html) {
    let items = this._filteredItems();
    return html`
      <h4>Monitoring: ${this.selected}</h4>
      <small>
        <table>
          <tbody>
            ${items.map((item) => html` ${this._row(html, item)} `)}
          </tbody>
        </table>
      </small>
    `;
  }
}

customElements.define("monitor-module", MonitorModule);
