import {useCookie, useFetch, type UseFetchOptions } from 'nuxt/app'

export function useApi<T>(url: string | (() => string), options: UseFetchOptions<T> = {}) {
  const token = useCookie('auth_token')

  const defaults: UseFetchOptions<T> = {
    headers: token.value ? { Authorization: `Bearer ${token.value}` } : {}
  }

  const params = { ...defaults, ...options }

  return useFetch(url, params)
}