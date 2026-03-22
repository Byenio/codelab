import {navigateTo} from "nuxt/app";

export const useAuth = () => {
    const user = useState<any | null>('auth_user', () => null)
    const token = useCookie('auth_token')

    const isLoggedIn = computed(() => !!token.value)

    async function fetchUser(manualToken?: string) {
        let activeToken = manualToken || token.value

        if (!activeToken) {
            user.value = null
            return null
        }

        activeToken = activeToken.replace(/['"]+/g, '').trim()

        try {
            const data = await $fetch('/api/v1/me', {
                headers: {
                    Authorization: `Bearer ${activeToken}`,
                    Accept: 'application/json'
                },
                retry: 0
            })
            user.value = data
            return data
        } catch (error) {
            token.value = null
            user.value = null
            return null
        }
    }

    async function logout() {
        token.value = null
        user.value = null

        // TODO: invalidate token
        /*
        try {
          await $fetch('/api/v1/logout', { method: 'POST' })
        } catch (e) {
          console.error("Backend logout failed, but clearing local session anyway.")
        }
        */

        await navigateTo('/login')

        const toast = useToast()
        toast.add({ title: 'Logged out successfully', color: 'neutral' })
    }

    return {
        user,
        token,
        isLoggedIn,
        fetchUser,
        logout
    }
}