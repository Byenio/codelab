<script setup lang="ts">
import { z } from 'zod'
import type { FormSubmitEvent } from '#ui/types'
import auth from "~~/middleware/auth";
import {useApi} from "~~/composables/useApi";

definePageMeta({
  middleware: auth
})

const form = reactive({
  name: '',
  description: '',
  is_private: false,
  auto_init: true
})

const isLoading = ref(false)
const toast = useToast()

async function onSubmit() {
  isLoading.value = true
  try {
    const { data } = await useApi('/api/v1/repositories', {
      method: 'POST',
      body: {
        name: form.name,
        description: form.description,
        is_private: form.is_private,
        init_readme: form.auto_init
      }
    })

    toast.add({
      title: 'Repository Created',
      description: `Successfully created ${form.name}`,
      color: 'success'
    })

    await navigateTo(`/${form.name}`)

  } catch (error: any) {
    toast.add({
      title: 'Creation Failed',
      description: error.data || 'Something went wrong',
      color: 'error'
    })
  } finally {
    isLoading.value = false
  }
}
</script>

<template>
  <div class="min-h-screen bg-zinc-50 dark:bg-zinc-950 py-12">
    <UContainer class="max-w-2xl">

      <div class="mb-8">
        <h1 class="text-3xl font-bold text-zinc-900 dark:text-white">Create a new repository</h1>
        <p class="text-zinc-500 mt-2">A repository contains all project files, including the revision history.</p>
      </div>

      <UCard>
        <form @submit.prevent="onSubmit" class="space-y-6">

          <!-- Repository Name -->
          <UFormField label="Repository Name" name="name" help="Great repository names are short and memorable.">
            <UInput
                v-model="form.name"
                placeholder="my-awesome-project"
                icon="i-heroicons-book-open"
            />
          </UFormField>

          <!-- Description -->
          <UFormField label="Description (Optional)" name="description">
            <UTextarea
                v-model="form.description"
                placeholder="What is this project about?"
                autoresize
            />
          </UFormField>

          <hr class="border-zinc-200 dark:border-zinc-800" />

          <!-- Visibility Options -->
          <div class="space-y-3">
            <label class="block text-sm font-medium text-zinc-700 dark:text-zinc-200">Visibility</label>

            <!-- Public Option -->
            <div
                class="flex items-start gap-3 p-3 border rounded-lg cursor-pointer transition-colors"
                :class="!form.is_private ? 'border-primary-500 bg-primary-50 dark:bg-primary-950/20' : 'border-zinc-200 dark:border-zinc-800'"
                @click="form.is_private = false"
            >
              <URadio v-model="form.is_private" :value="false" />
              <div>
                <div class="flex items-center gap-2 font-medium">
                  <UIcon name="i-heroicons-globe-alt" />
                  Public
                </div>
                <p class="text-sm text-zinc-500">Anyone on the internet can see this repository.</p>
              </div>
            </div>

            <!-- Private Option -->
            <div
                class="flex items-start gap-3 p-3 border rounded-lg cursor-pointer transition-colors"
                :class="form.is_private ? 'border-primary-500 bg-primary-50 dark:bg-primary-950/20' : 'border-zinc-200 dark:border-zinc-800'"
                @click="form.is_private = true"
            >
              <URadio v-model="form.is_private" :value="true" />
              <div>
                <div class="flex items-center gap-2 font-medium">
                  <UIcon name="i-heroicons-lock-closed" />
                  Private
                </div>
                <p class="text-sm text-zinc-500">You choose who can see and commit to this repository.</p>
              </div>
            </div>
          </div>

          <hr class="border-zinc-200 dark:border-zinc-800" />

          <!-- Initialization -->
          <UFormField>
            <UCheckbox
                v-model="form.auto_init"
                label="Initialize this repository with a README"
                help="This will let you immediately clone the repository to your computer."
            />
          </UFormField>

          <!-- Submit Button -->
          <div class="pt-4">
            <UButton
                type="submit"
                size="lg"
                block
                :loading="isLoading"
                :disabled="!form.name"
            >
              Create Repository
            </UButton>
          </div>

        </form>
      </UCard>

    </UContainer>
  </div>
</template>