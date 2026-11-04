(() => {
  'use strict';
  const data = JSON.parse(document.getElementById('page-data').textContent);
  const tabs = [...document.querySelectorAll('[data-tab]')];
  const image = document.getElementById('gallery-image');
  const caption = document.getElementById('gallery-caption');
  const panel = document.getElementById('gallery-panel');
  function selectTab(index, focus = false) {
    tabs.forEach((tab, n) => {tab.setAttribute('aria-selected', String(n === index)); tab.tabIndex = n === index ? 0 : -1;});
    image.src = data.images[index]; image.alt = data.alts[index];
    image.style.aspectRatio = index === 2 ? 'auto' : '1840 / 1300';
    caption.textContent = data.captions[index];
    panel.setAttribute('aria-labelledby', tabs[index].id);
    if (focus) tabs[index].focus();
  }
  tabs.forEach((tab, index) => {
    tab.addEventListener('click', () => selectTab(index));
    tab.addEventListener('keydown', (event) => {
      let next;
      if (event.key === 'ArrowRight') next = (index + 1) % tabs.length;
      if (event.key === 'ArrowLeft') next = (index + tabs.length - 1) % tabs.length;
      if (event.key === 'Home') next = 0;
      if (event.key === 'End') next = tabs.length - 1;
      if (next !== undefined) {event.preventDefault(); selectTab(next, true);}
    });
  });
  const lightbox = document.getElementById('lightbox');
  const enlarged = document.getElementById('lightbox-image');
  const close = document.getElementById('close-lightbox');
  const opener = document.getElementById('open-shot');
  let previousOverflow = '';
  function hideLightbox() {lightbox.hidden = true; document.body.style.overflow = previousOverflow; opener.focus();}
  opener.addEventListener('click', () => {
    enlarged.src = image.src; enlarged.alt = image.alt;
    previousOverflow = document.body.style.overflow; document.body.style.overflow = 'hidden';
    lightbox.hidden = false; close.focus();
  });
  close.addEventListener('click', hideLightbox);
  lightbox.addEventListener('click', event => {if (event.target === lightbox) hideLightbox();});
  lightbox.addEventListener('keydown', event => {
    if (event.key === 'Escape') hideLightbox();
    if (event.key === 'Tab') {event.preventDefault(); close.focus();}
  });
  let dskActive = false;
  const dsk = document.getElementById('toggle-dsk');
  const dskImage = document.getElementById('dsk-image');
  dsk.addEventListener('click', () => {
    dskActive = !dskActive; const state = Number(dskActive);
    dsk.setAttribute('aria-pressed', String(dskActive)); dsk.textContent = data.dskLabels[state];
    dskImage.src = data.dskImages[state]; dskImage.alt = data.dskAlts[state];
  });
  const video = document.getElementById('light-video');
  const motion = document.getElementById('toggle-motion');
  function motionLabel() {motion.textContent = video.paused ? data.play : data.pause;}
  video.addEventListener('play', motionLabel); video.addEventListener('pause', motionLabel);
  motion.addEventListener('click', () => {if (video.paused) video.play().catch(() => {}); else video.pause();});
  const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
  if (!reduceMotion.matches) video.play().catch(() => {});
  reduceMotion.addEventListener('change', event => {if (event.matches) video.pause();});
  document.addEventListener('visibilitychange', () => {if (document.hidden) video.pause();});
})();
