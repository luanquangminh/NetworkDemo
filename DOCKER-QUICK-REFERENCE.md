# Docker Quick Reference Card

Quick commands for Docker deployment of File Sharing Server.

## 🚀 Quick Start

```bash
# 1. Build image
make docker-build

# 2. Start server
make docker-run

# 3. Connect client
./build/gui_client
# Server: localhost:8080
# Login: admin/admin
```

---

## 📋 Common Commands

| Task | Command | Description |
|------|---------|-------------|
| Build image | `make docker-build` | Build server Docker image |
| Start server | `make docker-run` | Start container in background |
| Stop server | `make docker-stop` | Stop and remove container |
| Restart | `make docker-restart` | Restart running container |
| View logs | `make docker-logs` | Show real-time logs |
| Check status | `make docker-status` | Show container status |
| Open shell | `make docker-shell` | SSH into container |
| Clean up | `make docker-clean` | Remove container and image |

---

## 🔧 Configuration

### Default Settings

```yaml
Port:     8080
Database: ./data/fileshare.db
Storage:  ./storage/
Username: admin
Password: admin
```

### Change Port

Edit `docker-compose.yml`:
```yaml
ports:
  - "9090:8080"  # Change 9090 to desired port
```

Then restart:
```bash
make docker-restart
```

---

## 🌐 Client Connection

### From Same Machine
```
Server IP: localhost
Port:      8080
```

### From Another Machine
```bash
# Find server IP
ifconfig | grep "inet " | grep -v 127.0.0.1
```
```
Server IP: <your-ip-address>
Port:      8080
```

---

## 🗄️ Data Management

### Backup
```bash
tar -czf backup-$(date +%Y%m%d).tar.gz data/ storage/
```

### Restore
```bash
make docker-stop
tar -xzf backup-YYYYMMDD.tar.gz
make docker-run
```

### Reset Database
```bash
make docker-stop
rm -rf data/
make docker-run  # Creates fresh database
```

---

## 🐛 Troubleshooting

### Check Logs
```bash
make docker-logs
```

### Port Already in Use
```bash
# Kill process on port 8080
lsof -ti:8080 | xargs kill
```

### Container Won't Start
```bash
# View detailed status
docker ps -a

# Check logs
make docker-logs

# Rebuild
make docker-clean
make docker-build
make docker-run
```

### Database Issues
```bash
# Check database
docker exec fileshare-server sqlite3 /app/data/fileshare.db ".tables"

# Reset database
make docker-stop
rm data/fileshare.db
make docker-run
```

### Client Can't Connect
```bash
# Test connectivity
nc -zv localhost 8080

# Check container status
make docker-status

# Restart container
make docker-restart
```

---

## 📊 Monitoring

### View Resource Usage
```bash
docker stats fileshare-server
```

### Check Health
```bash
docker inspect fileshare-server | grep -A 5 Health
```

### Database Stats
```bash
docker exec fileshare-server sqlite3 /app/data/fileshare.db \
  "SELECT COUNT(*) as users FROM users; SELECT COUNT(*) as files FROM files;"
```

---

## 🔐 Security

### Change Admin Password

1. Connect as admin via client
2. Use admin dashboard to change password
3. Or manually update database:
```bash
docker exec fileshare-server sqlite3 /app/data/fileshare.db \
  "UPDATE users SET password_hash='<sha256-hash>' WHERE username='admin';"
```

---

## 🎯 Production Checklist

- [ ] Change default admin password
- [ ] Set up regular backups
- [ ] Configure firewall rules
- [ ] Enable log rotation
- [ ] Monitor resource usage
- [ ] Test disaster recovery
- [ ] Document server IP for clients

---

## 📚 Full Documentation

- Detailed guide: [README-DOCKER.md](README-DOCKER.md)
- Testing guide: [DOCKER-TESTING-GUIDE.md](DOCKER-TESTING-GUIDE.md)
- Project docs: [PROJECT_DOCUMENTATION.md](PROJECT_DOCUMENTATION.md)

---

## ⚡ One-Liner Commands

```bash
# Complete setup
make docker-build && make docker-run && make docker-status

# Full restart
make docker-stop && make docker-run

# Clean and rebuild
make docker-clean && make clean && make docker-build && make docker-run

# View live logs and status
make docker-logs &  # Terminal 1
watch -n 2 'make docker-status'  # Terminal 2

# Backup before maintenance
tar -czf backup-$(date +%Y%m%d-%H%M%S).tar.gz data/ storage/
```

---

## 🆘 Emergency Commands

```bash
# Force stop everything
docker stop fileshare-server
docker rm fileshare-server

# Nuclear option - remove everything
docker stop $(docker ps -aq)
docker rm $(docker ps -aq)
docker rmi fileshare-server

# Start fresh
make docker-build && make docker-run
```

---

## 📞 Support

1. Check logs: `make docker-logs`
2. Review docs: [README-DOCKER.md](README-DOCKER.md)
3. Test connectivity: `nc -zv localhost 8080`
4. Verify status: `make docker-status`
