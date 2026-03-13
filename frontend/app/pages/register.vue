<script setup lang="ts">
const form = reactive({
  username: '',
  email: '',
  password: ''
})

const isLoading = ref(false)
const toast = useToast()

async function handleRegister() {
  isLoading.value = true
  try {
    await $fetch('/api/v1/register', {
      method: 'POST',
      body: form
    })

    toast.add({
      title: 'Account Created',
      description: 'Please login with your new credentials.',
      color: 'success'
    })

    await navigateTo('/login')
  } catch (error: any) {
    toast.add({
      title: 'Registration Failed',
      description: error.data?.error || error.message,
      color: 'error'
    })
  } finally {
    isLoading.value = false
  }
}
</script>

<template>
  <div class="flex min-h-screen items-center justify-center bg-gray-50 dark:bg-gray-950 p-4">
    <UCard class="w-full max-w-md">
      <template #header>
        <div class="flex items-center gap-2">
          <UIcon name="i-heroicons-user-plus" class="text-primary-500 w-8 h-8" />
          <h1 class="text-xl font-bold">Create Account</h1>
        </div>
      </template>

      <form @submit.prevent="handleRegister" class="space-y-4">
        <UFormField label="Username" name="username">
          <UInput v-model="form.username" icon="i-heroicons-user" class="w-full" />
        </UFormField>

        <UFormField label="Email" name="email">
          <UInput v-model="form.email" type="email" icon="i-heroicons-envelope" class="w-full" />
        </UFormField>

        <UFormField label="Password" name="password">
          <UInput v-model="form.password" type="password" icon="i-heroicons-lock-closed" class="w-full" />
        </UFormField>

        <UButton type="submit" block :loading="isLoading" class="w-full">
          Create Account
        </UButton>
      </form>

      <template #footer>
        <p class="text-sm text-center text-gray-500">
          Already have an account?
          <NuxtLink to="/login" class="text-primary-500 hover:underline">
            Sign in
          </NuxtLink>
        </p>
      </template>
    </UCard>
  </div>
</template>