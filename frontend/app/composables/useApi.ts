import { useFetch, type UseFetchOptions } from 'nuxt/app'
import defu from "defu";

export function useApi<T>(url: string | (() => string), options: UseFetchOptions<T> = {}) {
  const { token, logout } = useAuth()

  const defaults: UseFetchOptions<T> = {
    headers: token.value
        ? { Authorization: `Bearer ${token.value}`, Accept: 'application/json' }
        : { Accept: 'application/json' },

    onResponseError({ response }) {
      if (response.status === 401) {
        logout()
      }
    }
  }

  const params = defu(options, defaults)

  return useFetch(url, params)
}