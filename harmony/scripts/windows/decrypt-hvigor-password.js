const crypto = require('crypto')
const fs = require('fs')
const path = require('path')

function readOnlyFile(directory) {
  const files = fs.readdirSync(directory)
  if (files.length !== 1) {
    throw new Error(`invalid Hvigor signing material directory: ${directory}`)
  }
  return fs.readFileSync(path.join(directory, files[0]))
}

function decrypt(key, data) {
  const payloadLength = data.readInt32BE(0)
  const ivLength = data.length - 4 - payloadLength
  const iv = data.subarray(4, 4 + ivLength)
  const body = data.subarray(4 + ivLength, data.length - 16)
  const decipher = crypto.createDecipheriv('aes-128-gcm', key, iv)
  decipher.setAuthTag(data.subarray(data.length - 16))
  return Buffer.concat([decipher.update(body), decipher.final()])
}

function decryptPassword(signingRoot, encryptedPassword) {
  const materialRoot = path.join(signingRoot, 'material')
  const parts = fs.readdirSync(path.join(materialRoot, 'fd'))
    .map((name) => readOnlyFile(path.join(materialRoot, 'fd', name)))
  parts.push(Buffer.from([49, 243, 9, 115, 214, 175, 91, 184, 211, 190, 177, 88, 101, 131, 192, 119]))
  const mixed = Buffer.from(parts[0])
  for (let part = 1; part < parts.length; part++) {
    for (let index = 0; index < mixed.length; index++) {
      mixed[index] ^= parts[part][index]
    }
  }
  const rootKey = crypto.pbkdf2Sync(
    mixed.toString(),
    readOnlyFile(path.join(materialRoot, 'ac')),
    10000,
    16,
    'sha256'
  )
  const workKey = decrypt(rootKey, readOnlyFile(path.join(materialRoot, 'ce')))
  return decrypt(workKey, Buffer.from(encryptedPassword, 'hex')).toString('utf8')
}

if (process.argv.length !== 4) {
  throw new Error('usage: decrypt-hvigor-password.js <signing-root> <encrypted-password>')
}
process.stdout.write(decryptPassword(path.resolve(process.argv[2]), process.argv[3]))
