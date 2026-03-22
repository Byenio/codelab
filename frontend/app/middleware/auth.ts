import {defineNuxtRouteMiddleware, navigateTo, useCookie} from "nuxt/app";

export default defineNuxtRouteMiddleware(async (to, from) => {
    const token = useCookie('auth_token');

    if (!token.value) {
        return navigateTo('/login');
    }

    try {
        await $fetch('/api/v1/me', {
            headers: {
                Authorization: `Bearer ${token.value}`
            },
            retry: 0
        })
    } catch (error: any) {
        token.value = null
        return navigateTo('/login')
    }
})