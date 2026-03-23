export const useAuth = () => {
    const user = useState<any | null>('auth_user', () => null)
    const token = useCookie('auth_token', {
        maxAge: 60 * 60 * 24 * 7, // 1 week
        watch: true,
        sameSite: 'lax'
    })

    async function fetchUser(manualToken?: string) {
        const activeToken = manualToken || token.value
        if (!activeToken) return null

        // Cache check: If we already have the user, don't hit the API again
        if (user.value && !manualToken) return user.value

        const cleanToken = activeToken.replace(/['"]+/g, '').trim()

        try {
            // We use the raw $fetch here for the initial check
            const data = await $fetch('/api/v1/me', {
                headers: {
                    Authorization: `Bearer ${cleanToken}`,
                    Accept: 'application/json'
                }
            })
            user.value = data
            return data
        } catch (error: any) {
            // CRITICAL: Only clear the session if the error is 401 (Unauthorized)
            // If the server is just down (500) or unreachable, don't log the user out!
            if (error.response?.status === 401) {
                console.error("Session expired or invalid token.")
                token.value = null
                user.value = null
            }
            return null
        }
    }

    async function logout() {
        token.value = null
        user.value = null
        await navigateTo('/login')
    }

    return {
        user,
        token,
        fetchUser,
        logout,
        isLoggedIn: computed(() => !!token.value)
    }
}