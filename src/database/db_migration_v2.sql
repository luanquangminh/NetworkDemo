-- Database Migration V2: Add Admin Support
-- This migration adds admin role support to the users table

-- Add is_admin column to users table
ALTER TABLE users ADD COLUMN is_admin INTEGER DEFAULT 0;

-- Make the default admin user (id=1) an admin
UPDATE users SET is_admin = 1 WHERE id = 1;

-- Create index for admin lookups
CREATE INDEX IF NOT EXISTS idx_users_admin ON users(is_admin);
