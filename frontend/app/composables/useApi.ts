// composables/useApi.ts
import { useFetch, type UseFetchOptions } from 'nuxt/app'
import defu from "defu";

export function useApi<T>(url: string | (() => string), options: UseFetchOptions<T> = {}) {
  const { token, logout } = useAuth()

  const defaults: UseFetchOptions<T> = {
    // This ensures headers are reactive and fresh for every request
    onRequest({ options }) {
      if (token.value) {
        const cleanToken = token.value.replace(/['"]+/g, '').trim()

        options.headers = (options.headers || {}) as any
        // Use set if it's a Headers object, or standard assignment if it's a literal
        if (options.headers instanceof Headers) {
          options.headers.set('Authorization', `Bearer ${cleanToken}`)
        } else {
          // @ts-ignore
          options.headers.Authorization = `Bearer ${cleanToken}`
        }
      }
    },

    onResponseError({ response }) {
      // Only trigger logout on 401.
      // If it's a 404 or 500, we want the user to stay on the page and see the error.
      if (response.status === 401) {
        console.warn("API 401 detected, logging out...");
        logout()
      }
    }
  }

  // Use defu to merge user options with our auth defaults
  const params = defu(options, defaults)

  return useFetch(url, params)
}