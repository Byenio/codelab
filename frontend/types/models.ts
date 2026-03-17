export interface User {
    id: number
    username: string
    email?: string
}

export interface Repository {
    id: number
    user_id: number
    directory_id?: number
    name: string
    description: string
    is_private: boolean
    created_at: string
    updated_at?: string
}

export interface Directory {
    id: number
    parent_id?: number
    name: string
}

export interface FileEntry {
    name: string
    path: string
    is_directory: boolean
    last_commit_msg: string
    last_commit_date: string
    size: number
}