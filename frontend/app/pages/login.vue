<script setup lang="ts">
import guest from "~/middleware/guest";

definePageMeta({
  middleware: guest
})

const form = reactive({
  username: '',
  password: ''
})

const isLoading = ref(false)
const toast = useToast()

async function handleLogin() {
  isLoading.value = true
  try {
    const response: any = await $fetch('/api/v1/login', {
      method: 'POST',
      body: {
        username: form.username,
        password: form.password
      }
    })

    const token = response.token
    if (token) {
      const cookie = useCookie('auth_token')
      cookie.value = token

      toast.add({
        title: 'Login Successful',
        description: 'Welcome back to CodeLab!',
        color: 'success'
      })

      await navigateTo('/dashboard')
    }
  } catch (error: any) {
    toast.add({
      title: 'Login Failed',
      description: error.data?.error || error.message,
      color: 'error'
    })
  } finally {
    isLoading.value = false
  }
}
</script>

<template>
  <div class="flex min-h-screen items-center justify-center bg-zinc-50 dark:bg-zinc-950 p-4">
    <UCard class="w-full max-w-md">
      <template #header>
        <div class="flex items-center gap-2">
          <UIcon name="i-heroicons-command-line" class="text-primary-500 w-8 h-8" />
          <h1 class="text-xl font-bold">Sign in to CodeLab</h1>
        </div>
      </template>

      <form @submit.prevent="handleLogin" method="POST" class="space-y-4">
        <UFormField label="Username" name="username">
          <UInput
              v-model="form.username"
              icon="i-heroicons-user"
              placeholder="gituser"
              autofocus
              class="w-full"
          />
        </UFormField>

        <UFormField label="Password" name="password">
          <UInput
              v-model="form.password"
              type="password"
              icon="i-heroicons-lock-closed"
              placeholder="••••••••"
              class="w-full"
          />
        </UFormField>

        <UButton
            type="submit"
            block
            :loading="isLoading"
            class="w-full"
        >
          Sign in
        </UButton>
      </form>

      <template #footer>
        <p class="text-sm text-center text-zinc-500">
          New to CodeLab?
          <NuxtLink to="/register" class="text-primary-500 hover:underline">
            Create an account
          </NuxtLink>
        </p>
      </template>
    </UCard>
  </div>
</template>