import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'

// ?debug keeps the raw serial test page around, loaded on demand so the two stylesheets never mix
const debug = new URLSearchParams(location.search).has('debug')
const page = debug ? import('./DebugApp.tsx') : import('./App.tsx')

void page.then(({ default: Page }) => {
  createRoot(document.getElementById('root')!).render(
    <StrictMode>
      <Page />
    </StrictMode>,
  )
})
