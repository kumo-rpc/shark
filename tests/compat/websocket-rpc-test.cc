// Copyright (c) 2021 Ian Denhardt and contributors
// Licensed under the MIT License:
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <shark/rpc/websocket-rpc.h>
#include <tests/base/test.h>

#include <tests/test.sk.h>

KJ_TEST("WebSocketMessageStream") {
  kj::EventLoop loop;
  kj::WaitScope waitScope(loop);

  auto pipe = kj::newWebSocketPipe();

  auto msgStreamA = shark::WebSocketMessageStream(*pipe.ends[0]);
  auto msgStreamB = shark::WebSocketMessageStream(*pipe.ends[1]);

  // Make a message, fill it with some stuff
  shark::MallocMessageBuilder originalMsg;
  auto object = originalMsg.initRoot<sharkproto_test::shark::test::TestAllTypes>().initStructList(10);
  object[0].setTextField("Test");
  object[1].initStructField().setTextField("A string");
  object[2].setTextField("Another field");
  object[3].setInt64Field(42);
  auto originalSegments = originalMsg.getSegmentsForOutput();

  // Send the message across the websocket, make sure it comes out unharmed.
  auto writePromise = msgStreamA.writeMessage(nullptr, originalSegments);
  msgStreamB.tryReadMessage(nullptr)
    .then([&](auto maybeResult) -> kj::Promise<void> {
      KJ_IF_MAYBE(result, maybeResult) {
        KJ_ASSERT(result->fds.size() == 0);
        KJ_ASSERT(result->reader->getSegment(originalSegments.size()) == nullptr);
        for(size_t i = 0; i < originalSegments.size(); i++) {
          auto oldSegment = originalSegments[i];
          auto newSegment = result->reader->getSegment(i);

          KJ_ASSERT(oldSegment.size() == newSegment.size());
          KJ_ASSERT(memcmp(
                &oldSegment[0],
                &newSegment[0],
                oldSegment.size() * sizeof(shark::word)
                ) == 0);
        }
        return kj::READY_NOW;
      } else {
        KJ_FAIL_ASSERT("Reading first message failed");
      }
  }).wait(waitScope);
  writePromise.wait(waitScope);

  // Close the websocket, and make sure the other end gets nullptr when reading.
  auto endPromise = msgStreamA.end();
  msgStreamB.tryReadMessage(nullptr).then([](auto maybe) -> kj::Promise<void> {
    KJ_IF_MAYBE(segments, maybe) {
      KJ_FAIL_ASSERT("Should have gotten nullptr after websocket was closed");
    }
    return kj::READY_NOW;
  }).wait(waitScope);
  endPromise.wait(waitScope);
}

KJ_TEST("WebSocketMessageStreamByteCount") {
  kj::EventLoop loop;
  kj::WaitScope waitScope(loop);

  auto pipe1 = kj::newWebSocketPipe();
  auto pipe2 = kj::newWebSocketPipe();

  auto msgStreamA = shark::WebSocketMessageStream(*pipe1.ends[0]);
  auto msgStreamB = shark::WebSocketMessageStream(*pipe2.ends[1]);

  auto pumpTask = pipe1.ends[1]->pumpTo(*pipe2.ends[0]);

  shark::MallocMessageBuilder originalMsg;
  auto object = originalMsg.initRoot<sharkproto_test::shark::test::TestAllTypes>().initStructList(10);
  object[0].setTextField("Test");
  object[1].initStructField().setTextField("A string");
  object[2].setTextField("Another field");
  object[3].setInt64Field(42);
  auto originalSegments = originalMsg.getSegmentsForOutput();

  auto writePromise = msgStreamA.writeMessage(nullptr, originalSegments);
  msgStreamB.tryReadMessage(nullptr).wait(waitScope);
  writePromise.wait(waitScope);

  auto endPromise = msgStreamA.end();
  msgStreamB.tryReadMessage(nullptr).wait(waitScope);
  pumpTask.wait(waitScope);
  endPromise.wait(waitScope);
  KJ_EXPECT(pipe1.ends[0]->sentByteCount() == 2585);
  KJ_EXPECT(pipe1.ends[1]->receivedByteCount() == 2585);
  KJ_EXPECT(pipe2.ends[0]->sentByteCount() == 2585);
  KJ_EXPECT(pipe2.ends[1]->receivedByteCount() == 2585);
}
