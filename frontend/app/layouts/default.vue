<script setup lang="ts">

const { user, logout } = useAuth()

const userHomeLink = computed(() => {
  return user.value ? `/u/${user.value.username}` : '/login'
})

const handleLogout = async () => {
  await logout()
}
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950">
    <!-- Navbar -->
    <header class="border-b border-zinc-200 dark:border-zinc-800 bg-white dark:bg-zinc-900">
      <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 h-16 flex items-center justify-between">
        <div class="flex items-center gap-4">
          <NuxtLink :to="userHomeLink" class="text-xl font-bold text-primary-500">CodeLab</NuxtLink>
          <nav class="flex gap-4 text-sm font-medium">
            <NuxtLink :to="userHomeLink" class="text-zinc-900 dark:text-white hover:text-primary-500">Root</NuxtLink>
            <NuxtLink to="/settings" class="text-zinc-500 hover:text-zinc-900 dark:hover:text-white">Settings</NuxtLink>
          </nav>
        </div>

        <UButton
            @click="handleLogout"
            variant="ghost"
            icon="i-heroicons-arrow-right-on-rectangle"
        >
          Logout
        </UButton>
      </div>
    </header>

    <!-- Page Content -->
    <slot />
  </div>
</template>