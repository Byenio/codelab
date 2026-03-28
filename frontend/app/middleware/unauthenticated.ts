import { defineNuxtRouteMiddleware, navigateTo, useCookie } from "nuxt/app"

export default defineNuxtRouteMiddleware(async (to, from) => {
    const { fetchUser } = useAuth()
    const user = await fetchUser()

    if (user) {
        navigateTo(`/u/${user.username}`)
    }
})