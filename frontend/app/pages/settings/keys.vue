<script setup lang="ts">
import { z } from 'zod'
import { useApi } from "~~/composables/useApi";

// Define Key Type
interface SSHKey {
  id: number;
  title: string;
  key: string;
  created_at: string;
  fingerprint?: string;
}

// Fetch Keys
const { data: keys, refresh } = await useApi<SSHKey[]>('/api/v1/user/keys')

// Form State
const isOpen = ref(false)
const isLoading = ref(false)
const form = reactive({
  title: '',
  key: ''
})

const toast = useToast()

// Add Key Action
async function addKey() {
  if (!form.title || !form.key) return;

  isLoading.value = true
  try {
    await useApi('/api/v1/user/keys', {
      method: 'POST',
      body: {
        title: form.title,
        key: form.key
      }
    })

    toast.add({ title: 'SSH Key Added', color: 'success' })
    isOpen.value = false
    form.title = ''
    form.key = ''
    refresh() // Reload list
  } catch (err: any) {
    toast.add({ title: 'Error', description: err.message || 'Invalid Key', color: 'error' })
  } finally {
    isLoading.value = false
  }
}

// Delete Key Action
async function deleteKey(id: number) {
  if (!confirm('Are you sure you want to delete this key?')) return;

  try {
    await useApi(`/api/v1/user/keys/${id}`, { method: 'DELETE' })
    toast.add({ title: 'Key Deleted', color: 'success' })
    refresh()
  } catch (err) {
    toast.add({ title: 'Failed to delete', color: 'error' })
  }
}
</script>

<template>
  <div class="space-y-6">
    <div class="flex justify-between items-center">
      <div>
        <h2 class="text-lg font-bold text-zinc-900 dark:text-white">SSH Keys</h2>
        <p class="text-sm text-zinc-500">Manage SSH keys to access your repositories via Git.</p>
      </div>
      <UButton icon="i-heroicons-plus" @click="isOpen = true">New SSH Key</UButton>
    </div>

    <!-- Key List -->
    <UCard>
      <div v-if="!keys || keys.length === 0" class="text-center py-8 text-zinc-500">
        No SSH keys found. Add one to start pushing code.
      </div>

      <div v-else class="divide-y divide-zinc-100 dark:divide-zinc-800">
        <div v-for="key in keys" :key="key.id" class="py-4 flex justify-between items-start">
          <div>
            <div class="flex items-center gap-2">
              <UIcon name="i-heroicons-key" class="text-zinc-400" />
              <span class="font-bold text-zinc-700 dark:text-zinc-200">{{ key.title }}</span>
            </div>
            <p class="text-xs text-zinc-500 font-mono mt-1 break-all line-clamp-1 max-w-lg">
              {{ key.key }}
            </p>
            <p class="text-xs text-zinc-400 mt-1">Added on {{ new Date(key.created_at).toLocaleDateString() }}</p>
          </div>
          <UButton
              color="error"
              variant="ghost"
              icon="i-heroicons-trash"
              size="xs"
              @click="deleteKey(key.id)"
          />
        </div>
      </div>
    </UCard>

    <!-- Add Key Card -->
    <UCard>
      <template #header>
        <h3 class="font-bold">Add New SSH Key</h3>
      </template>

      <form @submit.prevent="addKey" class="space-y-4">
        <UFormField label="Title" name="title">
          <UInput v-model="form.title" placeholder="e.g. My Laptop" autofocus />
        </UFormField>

        <UFormField label="Key" name="key" help="Starts with ssh-rsa, ssh-ed25519, etc.">
          <UTextarea
              v-model="form.key"
              placeholder="ssh-ed25519 AAAAC3Nz..."
              :rows="5"
              class="font-mono text-xs"
          />
        </UFormField>

        <div class="flex justify-end gap-2 mt-4">
          <UButton color="gray" variant="ghost" @click="isOpen = false">Cancel</UButton>
          <UButton type="submit" :loading="isLoading" :disabled="!form.title || !form.key">Add Key</UButton>
        </div>
      </form>
    </UCard>
  </div>
</template>