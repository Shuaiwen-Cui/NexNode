/*
 * <liftnode-viewer> — lightweight loader for the LiftNode interactive 3D exploded view.
 *
 * This file is tiny and safe to include on every page: it only registers the custom
 * element. The heavy part (three.js + scene, see core.js) is fetched lazily the first
 * time a <liftnode-viewer> element scrolls near the viewport.
 *
 * Attributes
 *   lang      "en" | "zh"            UI / label language (default: page language)
 *   theme     "auto" | "dark" | "light"   (auto follows MkDocs Material's palette)
 *   models    base URL of the STL folder (default: <site>/HARDWARE-ESP/FUSION/)
 *   brand     overlay title (default LIFTNODE); tagline overrides the subtitle
 *   capture   present → no toolbar, deterministic timeline driven by renderAt(t)
 *   eager     present → load immediately instead of waiting for visibility
 */
const CORE_URL = new URL('./core.js', import.meta.url).href;

const PLACEHOLDER = `
<style>
  :host { display:block; position:relative; height:560px; border-radius:14px; overflow:hidden;
          background:radial-gradient(120% 90% at 50% 35%, #263042 0%, #121722 45%, #07090d 100%); }
  .ph { position:absolute; inset:0; display:grid; place-items:center; color:rgba(232,237,245,.55);
        font:500 12px/1 Roboto, system-ui, sans-serif; letter-spacing:.14em; text-transform:uppercase; }
</style>
<div class="ph">LiftNode · 3D</div>`;

class LiftNodeViewer extends HTMLElement {
  connectedCallback() {
    if (this._started) return;
    this._started = true;
    if (!this.shadowRoot) this.attachShadow({ mode: 'open' }).innerHTML = PLACEHOLDER;

    const start = () => {
      if (this._app || this._loading) return;
      this._loading = import(CORE_URL)
        .then((m) => m.mount(this))
        .then((app) => {
          this._app = app;
          this.dispatchEvent(new CustomEvent('ready', { detail: app }));
          return app;
        })
        .catch((err) => {
          console.error('[liftnode-viewer]', err);
          const ph = this.shadowRoot.querySelector('.ph');
          if (ph) ph.textContent = '3D viewer failed to load';
        });
    };

    if (this.hasAttribute('eager') || this.hasAttribute('capture') || !('IntersectionObserver' in window)) {
      start();
    } else {
      const io = new IntersectionObserver((entries) => {
        if (entries.some((e) => e.isIntersecting)) { io.disconnect(); start(); }
      }, { rootMargin: '300px' });
      io.observe(this);
    }
  }

  disconnectedCallback() {
    if (this._app) { this._app.dispose(); this._app = null; this._loading = null; this._started = false; }
  }

  /** Resolves with the running app (exposes renderAt / setExplode / setMode …). */
  whenReady() { return this._loading || Promise.reject(new Error('viewer not started')); }
}

if (!customElements.get('liftnode-viewer')) customElements.define('liftnode-viewer', LiftNodeViewer);
