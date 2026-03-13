import {defineNuxtRouteMiddleware, navigateTo, useCookie} from "nuxt/app";

export default defineNuxtRouteMiddleware((to, from) => {
    const token = useCookie('auth_token');

    if (!token.value) {
        return navigateTo('/login');
    }
})